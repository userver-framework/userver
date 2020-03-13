#include <storages/redis/impl/request.hpp>

#include <storages/redis/impl/reply.hpp>
#include <storages/redis/impl/sentinel.hpp>
#include "redis.hpp"

namespace redis {

Request::Request(Sentinel& sentinel, CmdArgs&& args, const std::string& key,
                 bool master, const CommandControl& command_control,
                 bool skip_status) {
  CommandPtr command_ptr =
      PrepareRequest(std::forward<CmdArgs>(args), command_control, skip_status);
  sentinel.AsyncCommand(command_ptr, key, master);
}

Request::Request(Sentinel& sentinel, CmdArgs&& args, size_t shard, bool master,
                 const CommandControl& command_control, bool skip_status) {
  CommandPtr command_ptr =
      PrepareRequest(std::forward<CmdArgs>(args), command_control, skip_status);
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
                                   bool skip_status) {
  request_future_.until_ =
      std::chrono::steady_clock::now() + command_control.timeout_all;
  auto command = PrepareCommand(
      std::move(args),
      [skip_status](const CommandPtr&, ReplyPtr reply, ReplyPtrPromise& prom) {
        if (skip_status && reply->data.IsStatus()) return;
        prom.set_value(std::move(reply));
      },
      command_control);
  request_future_.ro_future_ = command->promise.get_future();
  return command;
}

ReplyPtr RequestFuture::Get() {
  return (ro_future_.wait_until(until_) == std::future_status::ready)
             ? ro_future_.get()
             : std::make_shared<Reply>(std::string(), nullptr,
                                       redis::REDIS_ERR_TIMEOUT);
}

ReplyPtr Request::Get() { return request_future_.Get(); }

RequestFuture&& Request::PopFuture() { return std::move(request_future_); }

}  // namespace redis
