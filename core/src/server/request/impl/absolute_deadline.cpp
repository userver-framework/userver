#include <userver/server/request/impl/absolute_deadline.hpp>

#include <string>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/from_string.hpp>

#include <dynamic_config/variables/USERVER_DEADLINE_PROPAGATION_ABSOLUTE_TIMESTAMP_ENABLED.hpp>
#include <dynamic_config/variables/USERVER_DEADLINE_PROPAGATION_CLOCK_SKEW_THRESHOLD_MS.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request::impl {

namespace {

bool IsClockSkewAcceptable(
    const TaskInheritedOriginalDeadline& absolute_tp,
    std::chrono::milliseconds deadline_duration,
    std::chrono::milliseconds threshold,
    tracing::Span* span
) {
    if (threshold == std::chrono::milliseconds{0}) {
        return true;
    }

    const auto request_start_system_time = span ? span->GetStartSystemTime() : utils::datetime::Now();
    const auto expected_timestamp =
        std::chrono::time_point_cast<std::chrono::microseconds>(request_start_system_time) + deadline_duration;

    const auto skew = std::chrono::abs(absolute_tp - expected_timestamp);
    const auto skew_ms = std::chrono::duration_cast<std::chrono::milliseconds>(skew);

    if (span != nullptr) {
        span->AddNonInheritableTag("dp_clock_skew_ms", skew_ms.count());
    }

    if (skew_ms > threshold) {
        LOG_LIMITED_INFO()
            << "Clock skew detected: " << skew_ms << ", threshold: " << threshold
            << ". Falling back to duration-based deadline propagation";
        return false;
    }

    return true;
}

}  // namespace

std::optional<TaskInheritedOriginalDeadline> ParseXRequestDeadlineString(const std::string& timestring) {
    if (timestring.empty()) {
        return std::nullopt;
    }

    const auto parsed = utils::FromStringNoThrow<std::uint64_t>(timestring);
    if (!parsed.has_value()) {
        LOG_LIMITED_INFO() << "Can't parse X-Request-Deadline: " << utils::ToString(parsed.error());
        return std::nullopt;
    }
    return TaskInheritedOriginalDeadline{std::chrono::microseconds{parsed.value()}};
}

std::optional<engine::Deadline> TryMakeDeadlinePreferringAbsoluteTimestamp(
    const std::optional<TaskInheritedOriginalDeadline>& absolute_original_deadline,
    const bool deadline_propagation_prefer_timestamp,
    const std::optional<std::chrono::milliseconds>& client_deadline_duration,
    const dynamic_config::Snapshot& config,
    tracing::Span* span
) {
    if (!absolute_original_deadline.has_value() || !deadline_propagation_prefer_timestamp ||
        !config[::dynamic_config::USERVER_DEADLINE_PROPAGATION_ABSOLUTE_TIMESTAMP_ENABLED])
    {
        return std::nullopt;
    }

    bool use_absolute = true;
    if (client_deadline_duration.has_value()) {
        use_absolute = IsClockSkewAcceptable(
            *absolute_original_deadline,
            *client_deadline_duration,
            config[::dynamic_config::USERVER_DEADLINE_PROPAGATION_CLOCK_SKEW_THRESHOLD_MS],
            span
        );
    }

    if (use_absolute) {
        return engine::Deadline::FromTimePoint(*absolute_original_deadline);
    }
    return std::nullopt;
}

}  // namespace server::request::impl

USERVER_NAMESPACE_END
