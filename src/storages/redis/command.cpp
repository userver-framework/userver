#include "command.hpp"

#include <engine/async_task.hpp>

namespace storages {
namespace redis {

CommandPtr PrepareCommand(CmdArgs&& args, Callback&& callback,
                          engine::TaskProcessor& task_processor,
                          const CommandControl& command_control) {
  auto res = std::make_shared<Command>();
  res->args = std::move(args);
  if (callback) {
    res->callback = [&task_processor,
                     callback = std::move(callback) ](ReplyPtr reply) mutable {
      new engine::AsyncTask<void>(task_processor, engine::Promise<void>(),
                                  std::move(callback), std::move(reply));
    };
  }
  res->control = command_control;
  res->counter = 0;
  res->asking = false;
  return res;
}

}  // namespace redis
}  // namespace storages
