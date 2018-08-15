#include "command.hpp"

#include <engine/async.hpp>

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
      engine::CriticalAsync(task_processor, std::move(callback),
                            std::move(reply))
          .Detach();
    };
  }
  res->control = command_control;
  res->counter = 0;
  res->asking = false;
  return res;
}

}  // namespace redis
}  // namespace storages
