#pragma once

#include <atomic>
#include <cstdint>

#include <userver/utils/flags.hpp>

USERVER_NAMESPACE_BEGIN

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
struct alignas(std::uint64_t) SleepState final {
  using Flags = utils::Flags<SleepFlags>;
  enum class Epoch : std::uint32_t {};

  // 'flags' must go before 'epoch' so that AtomicSleepState::Pack is a no-op.
  Flags flags;
  Epoch epoch;
};

class AtomicSleepState final {
 public:
  constexpr explicit AtomicSleepState(SleepState value) noexcept
      : impl_(Pack(value)) {}

  template <std::memory_order Order>
  SleepState Load() const noexcept {
    return Unpack(impl_.load(Order));
  }

  template <std::memory_order Order>
  SleepState Exchange(SleepState value) noexcept {
    return Unpack(impl_.exchange(Pack(value), Order));
  }

  template <std::memory_order Success, std::memory_order Failure>
  bool CompareExchangeWeak(SleepState& expected, SleepState desired) noexcept {
    std::uint64_t expected_raw = Pack(expected);
    const bool result = impl_.compare_exchange_weak(expected_raw, Pack(desired),
                                                    Success, Failure);
    expected = Unpack(expected_raw);
    return result;
  }

  template <std::memory_order Order>
  SleepState FetchOrFlags(SleepState::Flags flags) noexcept {
    return Unpack(impl_.fetch_or(flags.GetValue(), Order));
  }

  template <std::memory_order Order>
  void ClearFlags(SleepState::Flags flags) noexcept {
    impl_.fetch_and(~static_cast<std::uint64_t>(flags.GetValue()), Order);
  }

 private:
  static_assert(sizeof(SleepState::Flags::ValueType) == sizeof(std::uint32_t));
  static_assert(sizeof(SleepState::Epoch) == sizeof(std::uint32_t));

  static constexpr std::uint64_t Pack(SleepState value) noexcept {
    return (static_cast<std::uint64_t>(value.epoch) << 32) |
           value.flags.GetValue();
  }

  static constexpr SleepState Unpack(std::uint64_t value) noexcept {
    return SleepState{
        SleepState::Flags{SleepFlags{static_cast<std::uint32_t>(value)}},
        SleepState::Epoch{static_cast<std::uint32_t>(value >> 32)}};
  }

  static_assert(std::atomic<std::uint64_t>::is_always_lock_free);

  std::atomic<std::uint64_t> impl_;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
