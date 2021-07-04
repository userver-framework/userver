#include <userver/storages/redis/impl/command.hpp>

#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>

namespace redis {

Command::Command(CmdArgs&& args, ReplyCallback callback, CommandControl control,
                 int counter, bool asking, size_t instance_idx, bool redirected)
    : args(std::move(args)),
      callback(std::move(callback)),
      log_extra(PrepareLogExtra()),
      control(control),
      instance_idx(instance_idx),
      counter(counter),
      asking(asking),
      redirected(redirected) {}

Command::Command(CmdArgs&& args, ReplyCallbackEx&& callback_,
                 CommandControl control, int counter, bool asking)
    : args(std::move(args)),
      callback_ex(std::move(callback_)),  // TODO: move!
      log_extra(PrepareLogExtra()),
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
                                     asking, instance_idx, redirected);
  }
}

logging::LogExtra Command::PrepareLogExtra() {
  const auto* span = tracing::Span::CurrentSpanUnchecked();
  if (span) {
    return {
        {"trace_id", span->GetTraceId()},
        {"parent_id", span->GetParentId()},
        {"span_id", span->GetSpanId()},
        {"link", span->GetLink()},
    };
  } else {
    // Non-user requests (e.g. PING)
    return {};
  }
}

Command::~Command() {
  if (callback_ex && !executed) {
    LOG_ERROR() << "Command not executed";
  }
}

CommandPtr PrepareCommand(CmdArgs&& args, ReplyCallback callback,
                          const CommandControl& command_control, int counter,
                          bool asking, size_t instance_idx, bool redirected) {
  return std::make_shared<Command>(std::move(args), std::move(callback),
                                   command_control, counter, asking,
                                   instance_idx, redirected);
}

CommandPtr PrepareCommand(CmdArgs&& args, ReplyCallbackEx&& callback,
                          const CommandControl& command_control, int counter,
                          bool asking) {
  return std::make_shared<Command>(std::move(args), std::move(callback),
                                   command_control, counter, asking);
}

}  // namespace redis
