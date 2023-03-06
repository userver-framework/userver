#include <userver/storages/redis/impl/request.hpp>

#include <userver/tracing/in_place_span.hpp>

#include <userver/storages/redis/impl/exception.hpp>
#include <userver/storages/redis/impl/reply.hpp>

#include <storages/redis/impl/command.hpp>
#include <storages/redis/impl/sentinel.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

namespace {

class ReplyState {
 public:
  explicit ReplyState(std::string&& span_name) : span_(std::move(span_name)) {
    span_.Get().DetachFromCoroStack();
  }

  ~ReplyState() {
    if (!executed_) {
      LOG_WARNING() << "A request has been dropped";
    }
  }

  engine::Promise<ReplyPtr>& Promise() { return promise_; }
  tracing::Span& Span() { return span_.Get(); }
  size_t GetRepliesToSkip() const { return replies_to_skip_; }
  void SetRepliesToSkip(size_t value) { replies_to_skip_ = value; }
  void SetExecuted() { executed_ = true; }

 private:
  engine::Promise<ReplyPtr> promise_;
  tracing::InPlaceSpan span_;
  size_t replies_to_skip_{0};
  bool executed_{false};
};

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
  sentinel.AsyncCommand(std::move(command_ptr), key, master);
}

Request::Request(Sentinel& sentinel, CmdArgs&& args, size_t shard, bool master,
                 const CommandControl& command_control,
                 size_t replies_to_skip) {
  CommandPtr command_ptr = PrepareRequest(std::forward<CmdArgs>(args),
                                          command_control, replies_to_skip);
  sentinel.AsyncCommand(std::move(command_ptr), master, shard);
}

CommandPtr Request::PrepareRequest(CmdArgs&& args,
                                   const CommandControl& command_control,
                                   size_t replies_to_skip) {
  deadline_ = engine::Deadline::FromDuration(command_control.timeout_all);

  // Sadly, we don't have std::move_only_function, so we need a shared_ptr.
  auto state_ptr = std::make_shared<ReplyState>(MakeSpanName(args));
  state_ptr->SetRepliesToSkip(replies_to_skip);
  future_ = state_ptr->Promise().get_future();

  auto command = PrepareCommand(
      std::move(args),
      [state_ptr = std::move(state_ptr)](const CommandPtr&,
                                         ReplyPtr reply) mutable {
        if (!state_ptr) {
          LOG_LIMITED_WARNING() << "redis::Command keeps running after "
                                   "triggering the callback initially";
          return;
        }

        state_ptr->SetExecuted();

        if (state_ptr->GetRepliesToSkip() != 0) {
          state_ptr->SetRepliesToSkip(state_ptr->GetRepliesToSkip() - 1);
          if (reply->data.IsStatus()) return;
        }

        reply->FillSpanTags(state_ptr->Span());
        LOG_TRACE() << "Got reply from redis" << state_ptr->Span();

        state_ptr->Promise().set_value(std::move(reply));
        state_ptr.reset();
      },
      command_control);
  return command;
}

ReplyPtr Request::Get() {
  switch (future_.wait_until(deadline_)) {
    case engine::FutureStatus::kReady:
      return future_.get();

    case engine::FutureStatus::kTimeout:
      return std::make_shared<Reply>(std::string(), nullptr,
                                     ReplyStatus::kTimeoutError);

    case engine::FutureStatus::kCancelled:
      throw RequestCancelledException(
          "Redis request wait was aborted due to task cancellation");
  }
  UINVARIANT(false, "Invalid FutureStatus enum value");
}

}  // namespace redis

USERVER_NAMESPACE_END
