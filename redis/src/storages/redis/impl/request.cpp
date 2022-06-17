#include <userver/storages/redis/impl/request.hpp>

#include <userver/tracing/span.hpp>

#include <userver/storages/redis/impl/exception.hpp>
#include <userver/storages/redis/impl/reply.hpp>
#include <userver/storages/redis/impl/sentinel.hpp>
#include "redis.hpp"

USERVER_NAMESPACE_BEGIN

namespace redis {

namespace {
std::string MakeSpanName(const CmdArgs& cmd_args) {
  if (cmd_args.args.empty() || cmd_args.args.front().empty()) {
    return "redis_unknown";
  }

  if (cmd_args.args.size() > 1) {
    return "redis_multi";
  }

  return "redis_" + cmd_args.args.front().front();
}
}  // namespace

Request::Request(Sentinel& sentinel, CmdArgs&& args, const std::string& key,
                 bool master, const CommandControl& command_control,
                 size_t replies_to_skip) {
  CommandPtr command_ptr = PrepareRequest(std::forward<CmdArgs>(args),
                                          command_control, replies_to_skip);
  sentinel.AsyncCommand(command_ptr, key, master);
}

Request::Request(Sentinel& sentinel, CmdArgs&& args, size_t shard, bool master,
                 const CommandControl& command_control,
                 size_t replies_to_skip) {
  CommandPtr command_ptr = PrepareRequest(std::forward<CmdArgs>(args),
                                          command_control, replies_to_skip);
  sentinel.AsyncCommand(command_ptr, master, shard);
}

Request::Request(Sentinel& sentinel, CmdArgs&& args, size_t shard, bool master,
                 const CommandControl& command_control,
                 redis::ReplyCallback&& callback) {
  CommandPtr command_ptr = PrepareCommand(std::forward<CmdArgs>(args),
                                          std::move(callback), command_control);
  sentinel.AsyncCommand(command_ptr, master, shard);
}

CommandPtr Request::PrepareRequest(CmdArgs&& args,
                                   const CommandControl& command_control,
                                   size_t replies_to_skip) {
  request_future_.until_ =
      std::chrono::steady_clock::now() + command_control.timeout_all;
  request_future_.span_ptr_ =
      std::make_shared<tracing::Span>(MakeSpanName(args));
  request_future_.span_ptr_->DetachFromCoroStack();
  auto command = PrepareCommand(
      std::move(args),
      [replies_to_skip, span_ptr = request_future_.span_ptr_](
          const CommandPtr&, ReplyPtr reply, ReplyPtrPromise& prom) mutable {
        if (replies_to_skip) {
          --replies_to_skip;
          if (reply->data.IsStatus()) return;
        }
        if (span_ptr) {
          reply->FillSpanTags(*span_ptr);
          LOG_TRACE() << "Got reply from redis" << *span_ptr;
        }
        prom.set_value(std::move(reply));
      },
      command_control);
  request_future_.ro_future_ = command->promise.get_future();
  return command;
}

ReplyPtr RequestFuture::Get() {
  const auto status = ro_future_.wait_until(until_);
  if (status == engine::FutureStatus::kCancelled) {
    throw RequestCancelledException(
        "Redis request wait was aborted due to task cancellation");
  }
  auto reply_ptr =
      (status == engine::FutureStatus::kReady)
          ? ro_future_.get()
          : std::make_shared<Reply>(std::string(), nullptr, REDIS_ERR_TIMEOUT);
  span_ptr_.reset();
  return reply_ptr;
}

ReplyPtr Request::Get() { return request_future_.Get(); }

RequestFuture&& Request::PopFuture() { return std::move(request_future_); }

}  // namespace redis

USERVER_NAMESPACE_END
