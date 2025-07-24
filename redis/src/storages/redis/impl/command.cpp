#include <storages/redis/impl/command.hpp>

#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/assert.hpp>

#include <algorithm>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

namespace {
std::string ToLower(std::string_view str) {
    std::string result;
    result.resize(str.size());
    std::transform(str.begin(), str.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
    return result;
}

}  // namespace

Command::Command(
    CmdArgs&& _args,
    ReplyCallback callback,
    CommandControl control,
    int counter,
    bool asking,
    size_t instance_idx,
    bool redirected,
    bool read_only
)
    : args(std::move(_args)),
      callback(std::move(callback)),
      log_extra(PrepareLogExtra()),
      control(control),
      instance_idx(instance_idx),
      counter(counter),
      asking(asking),
      redirected(redirected),
      read_only(read_only),
      name(ToLower(args.GetCommandName(0))) {
    if constexpr (utils::impl::kEnableAssert) {
        original_span_debug = tracing::Span::CurrentSpanUnchecked();
    }
}

logging::LogExtra Command::PrepareLogExtra() {
    const auto* span = tracing::Span::CurrentSpanUnchecked();
    if (span) {
        return {
            {"trace_id", std::string{span->GetTraceId()}},
            {"parent_id", std::string{span->GetParentId()}},
            {"span_id", std::string{span->GetSpanId()}},
            {"link", std::string{span->GetLink()}},
        };
    } else {
        // Non-user requests (e.g. PING)
        return {};
    }
}

const logging::LogExtra& Command::GetLogExtra() const {
    const auto* span = tracing::Span::CurrentSpanUnchecked();
    if (span) {
        UASSERT_MSG(
            span == original_span_debug,
            "Work on the command is expected to be performed either "
            "in the Span that constructed it, or in an ev thread."
        );
        return logging::kEmptyLogExtra;
    } else {
        return log_extra;
    }
}

CommandPtr PrepareCommand(
    CmdArgs&& args,
    ReplyCallback callback,
    const CommandControl& command_control,
    int counter,
    bool asking,
    size_t instance_idx,
    bool redirected,
    bool read_only
) {
    return std::make_shared<Command>(
        std::move(args),
        std::move(callback),
        command_control,
        counter ? counter : command_control.retry_counter,
        asking,
        instance_idx,
        redirected,
        read_only
    );
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
