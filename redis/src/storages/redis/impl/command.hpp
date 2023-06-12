#pragma once

#include <userver/logging/log_extra.hpp>

#include <userver/storages/redis/impl/base.hpp>
#include <userver/storages/redis/impl/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

using ReplyCallback =
    std::function<void(const CommandPtr& cmd, ReplyPtr reply)>;

struct Command : public std::enable_shared_from_this<Command> {
  Command(CmdArgs&& _args, ReplyCallback callback, CommandControl control,
          int counter, bool asking, size_t instance_idx, bool redirected,
          bool read_only);

  const std::string& GetName() const { return name; }

  ReplyCallback Callback() const;

  void ResetStartHandlingTime() {
    start_handling_time = std::chrono::steady_clock::now();
  }

  std::chrono::steady_clock::time_point GetStartHandlingTime() const {
    return start_handling_time;
  }

  static logging::LogExtra PrepareLogExtra();

  CmdArgs args;
  ReplyCallback callback;
  std::chrono::steady_clock::time_point start_handling_time;
  logging::LogExtra log_extra;
  CommandControl control;
  size_t instance_idx = 0;
  uint32_t invoke_counter = 0;
  int counter = 0;
  bool asking = false;
  bool redirected = false;
  bool read_only = false;
  std::string name;
};

CommandPtr PrepareCommand(
    CmdArgs&& args, ReplyCallback callback,
    const CommandControl& command_control = kDefaultCommandControl,
    int counter = 0, bool asking = false, size_t instance_idx = 0,
    bool redirected = false, bool read_only = false);

}  // namespace redis

USERVER_NAMESPACE_END
