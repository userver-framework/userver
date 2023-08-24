#pragma once

#include <cstddef>
#include <utility>

USERVER_NAMESPACE_BEGIN

namespace engine::coro {

struct PoolStats {
  size_t active_coroutines = 0;
  size_t total_coroutines = 0;
};

inline PoolStats& operator+=(PoolStats& lhs, const PoolStats& rhs) {
  lhs.active_coroutines += rhs.active_coroutines;
  lhs.total_coroutines += rhs.total_coroutines;
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
