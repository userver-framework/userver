#include <storages/redis/impl/command.hpp>

#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

Command::Command(CmdArgs&& args, ReplyCallback callback, CommandControl control,
                 int counter, bool asking, size_t instance_idx, bool redirected,
                 bool read_only)
    : args(std::move(args)),
      callback(std::move(callback)),
      log_extra(PrepareLogExtra()),
      control(control),
      instance_idx(instance_idx),
      counter(counter),
      asking(asking),
      redirected(redirected),
      read_only(read_only) {}

ReplyCallback Command::Callback() const {
  auto self = shared_from_this();
  return [self](const CommandPtr& cmd, ReplyPtr reply) {
    if (self->callback) self->callback(cmd, std::move(reply));
  };
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

CommandPtr PrepareCommand(CmdArgs&& args, ReplyCallback callback,
                          const CommandControl& command_control, int counter,
                          bool asking, size_t instance_idx, bool redirected,
                          bool read_only) {
  return std::make_shared<Command>(std::move(args), std::move(callback),
                                   command_control, counter, asking,
                                   instance_idx, redirected, read_only);
}

}  // namespace redis

USERVER_NAMESPACE_END
