#pragma once

#include <cstddef>

USERVER_NAMESPACE_BEGIN

namespace rcu::impl {

using ThreadId = std::size_t;

constexpr ThreadId kMaxThreadCount = 1048568;

/// @returns the largest currently used ThreadId + 1
/// @note this number can only grow over time and is <= kMaxThreadCount
ThreadId GetThreadCount() noexcept;

/// @returns id of the current thread, < kMaxThreadCount
ThreadId GetCurrentThreadId() noexcept;

}  // namespace rcu::impl

USERVER_NAMESPACE_END
