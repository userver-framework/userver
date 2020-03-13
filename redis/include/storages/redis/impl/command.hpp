#pragma once

#include <storages/redis/impl/base.hpp>
#include <storages/redis/impl/types.hpp>

namespace redis {

struct Command : public std::enable_shared_from_this<Command> {
  Command(CmdArgs&& args, ReplyCallback callback, CommandControl control,
          int counter, bool asking, size_t instance_idx,
          const CommandPtr& parent);

  Command(CmdArgs&& args, ReplyCallbackEx&& callback, CommandControl control,
          int counter, bool asking);

  ~Command();

  Command(Command&& cmd) = delete;
  Command(const Command&) = delete;
  Command& operator=(const Command&) = delete;
  Command& operator=(Command&&) = delete;

  CmdArgs args;
  ReplyPtrPromise promise;  // FIXME: hack!!

  ReplyCallback Callback() const;
  void ResetStartHandlingTime() {
    start_handling_time = std::chrono::steady_clock::now();
  }
  std::chrono::steady_clock::time_point GetStartHandlingTime() const {
    return start_handling_time;
  }

  std::shared_ptr<Command> Clone() const;

 private:
  ReplyCallbackEx callback_ex;
  ReplyCallback callback;
  std::chrono::steady_clock::time_point start_handling_time;

 public:
  CommandControl control;
  int counter = 0;
  bool asking = false;
  size_t instance_idx = 0;
  bool executed = false;
};

CommandPtr PrepareCommand(
    CmdArgs&& args, ReplyCallback callback,
    const CommandControl& command_control = command_control_init,
    int counter = 0, bool asking = false, size_t instance_idx = 0,
    const CommandPtr& parent = CommandPtr());

CommandPtr PrepareCommand(CmdArgs&& args, ReplyCallbackEx&& callback,
                          const CommandControl& command_control,
                          int counter = 0, bool asking = false);

}  // namespace redis
