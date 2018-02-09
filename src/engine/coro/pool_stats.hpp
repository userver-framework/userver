#pragma once

#include <cstddef>

namespace engine {
namespace coro {

struct PoolStats {
  size_t active_coroutines = 0;
  size_t total_coroutines = 0;
};

PoolStats& operator+=(PoolStats& lhs, const PoolStats& rhs) {
  lhs.active_coroutines += rhs.active_coroutines;
  lhs.total_coroutines += rhs.total_coroutines;
  return lhs;
}

PoolStats operator+(const PoolStats& lhs, const PoolStats& rhs) {
  PoolStats sum = lhs;
  sum += rhs;
  return sum;
}

}  // namespace coro
}  // namespace engine
