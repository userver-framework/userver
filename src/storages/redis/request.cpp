#include "request.hpp"

#include "sentinel.hpp"

namespace storages {
namespace redis {

ReplyPtr Request::Get() {
  return (ro_future_.WaitUntil(until_) == std::future_status::ready)
             ? ro_future_.Get()
             : std::make_shared<Reply>(std::string(), nullptr,
                                       redis::REDIS_ERR_TIMEOUT);
}

CommandPtr Request::PrepareRequest(CmdArgs&& args,
                                   const CommandControl& command_control,
                                   bool skip_status) {
  std::shared_ptr<engine::Promise<ReplyPtr>> ro_prom_ptr =
      std::make_shared<engine::Promise<ReplyPtr>>();
  ro_future_ = ro_prom_ptr->GetFuture();
  until_ = std::chrono::steady_clock::now() + command_control.timeout_all;

  return PrepareCommand(std::move(args),
                        [ro_prom_ptr, skip_status](ReplyPtr reply) {
                          if (skip_status && reply->data.IsStatus()) return;
                          ro_prom_ptr->SetValue(reply);
                        },
                        command_control);
}

Request::Request(Sentinel& sentinel, CmdArgs&& args, const std::string& key,
                 bool master, const CommandControl& command_control,
                 bool skip_status) {
  CommandPtr command_ptr =
      PrepareRequest(std::move(args), command_control, skip_status);
  sentinel.AsyncCommand(command_ptr, key, master);
}

Request::Request(Sentinel& sentinel, CmdArgs&& args, size_t shard, bool master,
                 const CommandControl& command_control, bool skip_status) {
  CommandPtr command_ptr =
      PrepareRequest(std::move(args), command_control, skip_status);
  sentinel.AsyncCommand(command_ptr, master, shard);
}

}  // namespace redis
}  // namespace storages
