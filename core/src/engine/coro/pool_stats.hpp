#pragma once

#include <cstddef>
#include <utility>

USERVER_NAMESPACE_BEGIN

namespace engine::coro {

struct PoolStats {
  size_t active_coroutines = 0;
  size_t total_coroutines = 0;
  std::uint16_t max_stack_usage_pct = 0;
  bool is_stack_usage_monitor_active = false;
};

inline PoolStats& operator+=(PoolStats& lhs, const PoolStats& rhs) {
  lhs.active_coroutines += rhs.active_coroutines;
  lhs.total_coroutines += rhs.total_coroutines;
  if (rhs.max_stack_usage_pct > lhs.max_stack_usage_pct) {
    lhs.max_stack_usage_pct = rhs.max_stack_usage_pct;
  }
  lhs.is_stack_usage_monitor_active |= rhs.is_stack_usage_monitor_active;
  return lhs;
}

inline PoolStats operator+(PoolStats&& lhs, const PoolStats& rhs) {
  return std::move(lhs += rhs);
}

inline PoolStats operator+(const PoolStats& lhs, const PoolStats& rhs) {
  return PoolStats(lhs) + rhs;
}

}  // namespace engine::coro

USERVER_NAMESPACE_END
