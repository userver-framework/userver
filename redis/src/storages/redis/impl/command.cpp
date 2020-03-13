#include <storages/redis/impl/command.hpp>

#include <logging/log.hpp>

namespace redis {

Command::Command(CmdArgs&& args, ReplyCallback callback, CommandControl control,
                 int counter, bool asking, size_t instance_idx,
                 const CommandPtr& /*parent*/)
    : args(std::move(args)),
      callback(std::move(callback)),
      control(control),
      counter(counter),
      asking(asking),
      instance_idx(instance_idx) {}

Command::Command(CmdArgs&& args, ReplyCallbackEx&& callback_,
                 CommandControl control, int counter, bool asking)
    : args(std::move(args)),
      callback_ex(std::move(callback_)),  // TODO: move!
      control(control),
      counter(counter),
      asking(asking) {
  callback = [this](const CommandPtr& cmd, ReplyPtr reply) {
    callback_ex(cmd, std::move(reply), promise);
    executed = true;
  };
}

ReplyCallback Command::Callback() const {
  auto self = shared_from_this();
  return [self](const CommandPtr& cmd, ReplyPtr reply) {
    if (self->callback) self->callback(cmd, std::move(reply));
  };
}

std::shared_ptr<Command> Command::Clone() const {
  if (callback_ex) {
    return std::make_shared<Command>(args.Clone(), ReplyCallbackEx(callback_ex),
                                     control, counter, asking);
  } else {
    return std::make_shared<Command>(args.Clone(), callback, control, counter,
                                     asking, instance_idx, CommandPtr());
  }
}

Command::~Command() {
  if (callback_ex && !executed) {
    LOG_ERROR() << "Command not executed";
  }
}

CommandPtr PrepareCommand(CmdArgs&& args, ReplyCallback callback,
                          const CommandControl& command_control, int counter,
                          bool asking, size_t instance_idx,
                          const CommandPtr& parent) {
  return std::make_shared<Command>(std::move(args), std::move(callback),
                                   command_control, counter, asking,
                                   instance_idx, parent);
}

CommandPtr PrepareCommand(CmdArgs&& args, ReplyCallbackEx&& callback,
                          const CommandControl& command_control, int counter,
                          bool asking) {
  return std::make_shared<Command>(std::move(args), std::move(callback),
                                   command_control, counter, asking);
}

}  // namespace redis
