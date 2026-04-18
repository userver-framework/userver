#pragma once

/// @file userver/server/request/impl/absolute_deadline.hpp
/// @brief Absolute deadline parsing and engine deadline construction for propagation.

#include <chrono>
#include <optional>
#include <string>

#include <userver/dynamic_config/fwd.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/tracing/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

using TaskInheritedOriginalDeadline = std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>;

namespace impl {

std::optional<TaskInheritedOriginalDeadline> ParseXRequestDeadlineString(const std::string& timestring);

std::optional<engine::Deadline> TryMakeDeadlinePreferringAbsoluteTimestamp(
    const std::optional<TaskInheritedOriginalDeadline>& absolute_original_deadline,
    bool deadline_propagation_prefer_timestamp,
    const std::optional<std::chrono::milliseconds>& client_deadline_duration,
    const dynamic_config::Snapshot& config,
    tracing::Span* span
);

}  // namespace impl

}  // namespace server::request

USERVER_NAMESPACE_END
