#pragma once

#include <userver/logging/log_extra.hpp>

#include <userver/storages/redis/impl/base.hpp>
#include <userver/storages/redis/impl/types.hpp>

namespace redis {

struct Command : public std::enable_shared_from_this<Command> {
  Command(CmdArgs&& args, ReplyCallback callback, CommandControl control,
          int counter, bool asking, size_t instance_idx, bool redirected);

  Command(CmdArgs&& args, ReplyCallbackEx&& callback, CommandControl control,
          int counter, bool asking);

  ~Command();

  Command(Command&& cmd) = delete;
  Command(const Command&) = delete;
  Command& operator=(const Command&) = delete;
  Command& operator=(Command&&) = delete;

  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  CmdArgs args;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
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

  static logging::LogExtra PrepareLogExtra();

 public:
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  logging::LogExtra log_extra;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  CommandControl control;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  size_t instance_idx = 0;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  uint32_t invoke_counter = 0;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  int counter = 0;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  bool asking = false;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  bool executed = false;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  bool redirected = false;
};

CommandPtr PrepareCommand(
    CmdArgs&& args, ReplyCallback callback,
    const CommandControl& command_control = command_control_init,
    int counter = 0, bool asking = false, size_t instance_idx = 0,
    bool redirected = false);

CommandPtr PrepareCommand(CmdArgs&& args, ReplyCallbackEx&& callback,
                          const CommandControl& command_control,
                          int counter = 0, bool asking = false);

}  // namespace redis
