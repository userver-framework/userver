#pragma once

#include <functional>
#include <memory>

#include <engine/task/task_processor.hpp>

#include "base.hpp"
#include "reply.hpp"

namespace storages {
namespace redis {

using Callback = std::function<void(ReplyPtr request)>;

struct Command {
  CmdArgs args;
  Callback callback;
  CommandControl control;
  int counter = 0;
  bool asking = false;
  size_t instance_idx = -1;
};

using CommandPtr = std::shared_ptr<Command>;

template <typename Callback>
CommandPtr PrepareCommand(
    CmdArgs args, Callback&& callback,
    const CommandControl& command_control = command_control_init,
    int counter = 0, bool asking = false) {
  auto res = std::make_shared<Command>();
  res->args = std::move(args);
  res->callback = std::forward<Callback>(callback);
  res->control = command_control;
  res->counter = counter;
  res->asking = asking;
  return res;
}

CommandPtr PrepareCommand(
    CmdArgs&& args, Callback&& callback, engine::TaskProcessor& task_processor,
    const CommandControl& command_control = command_control_init);

}  // namespace redis
}  // namespace storages
