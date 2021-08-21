#pragma once

#include <atomic>
#include <cstdint>

#include <userver/utils/flags.hpp>

namespace engine::impl {

enum class SleepFlags : std::uint32_t {
  kNone = 0,
  kWakeupByWaitList = 1 << 1,
  kWakeupByDeadlineTimer = 1 << 2,
  kWakeupByCancelRequest = 1 << 3,
  kWakeupByBootstrap = 1 << 4,
  kSleeping = 1 << 5,
  kNonCancellable = 1 << 6,
};

// Without alignas(std::uint64_t) clang generates inefficient code with function
// calls while GCC outputs code with unaligned atomic operations.
struct alignas(std::uint64_t) SleepState {
  enum class Epoch : std::uint32_t {};
  using Flags = utils::Flags<SleepFlags>;

  Epoch epoch;
  Flags flags;
};

}  // namespace engine::impl
