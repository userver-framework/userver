#pragma once

#include <chrono>

#include <userver/engine/deadline.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

/// Default upper bound for a single statement / pool-acquire when no explicit deadline is given.
/// Milliseconds so defaults and settings can be sub-second; pool wait / @ref engine::Deadline use full resolution.
inline constexpr std::chrono::milliseconds kDefaultStatementTimeout{10000};

/// Combines task-inherited request deadline (if any) with \a operation_deadline, returning the
/// earlier of the two (like storages::postgres::AdjustTimeout).
engine::Deadline MergeWithInheritedDeadline(engine::Deadline operation_deadline) noexcept;

/// `MergeWithInheritedDeadline(Deadline::FromDuration(default_operation_budget))`.
/// Any `std::chrono::duration` works (e.g. milliseconds, or `duration<double, std::milli>` for fractional ms).
template <typename Rep, typename Period>
engine::Deadline GetExecuteDeadline(std::chrono::duration<Rep, Period> default_operation_budget) noexcept {
    return MergeWithInheritedDeadline(engine::Deadline::FromDuration(default_operation_budget));
}

void CheckDeadlineNotExpired(const engine::Deadline& deadline);

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
