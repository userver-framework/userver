#include <rcu/rcu.hpp>

#include <atomic>

namespace rcu::impl {

uint64_t GetNextEpoch() noexcept {
  static std::atomic<uint64_t> counter{1};  // 0 is the default value in data
  return counter++;
}

}  // namespace rcu::impl
