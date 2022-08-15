#pragma once

#include <cstdint>
#include <type_traits>

#include <fmt/core.h>
#include <boost/atomic/atomic.hpp>

#include <engine/task/sleep_state.hpp>
#include <userver/utils/underlying_value.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskContext;

struct alignas(8) Waiter32 {
  TaskContext* context{nullptr};
  SleepState::Epoch epoch{0};
};

struct alignas(16) Waiter64 {
  TaskContext* context{nullptr};
  SleepState::Epoch epoch{0};
  [[maybe_unused]] std::uint32_t padding_dont_use{0};
};

using Waiter = std::conditional_t<sizeof(void*) == 8, Waiter64, Waiter32>;

class AtomicWaiter final {
 public:
  AtomicWaiter() noexcept;

  bool IsEmptyRelaxed() noexcept;
  void Set(Waiter new_value) noexcept;
  bool ResetIfEquals(Waiter expected) noexcept;
  Waiter GetAndReset() noexcept;

 private:
  boost::atomic<Waiter> waiter_;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END

template <>
struct fmt::formatter<USERVER_NAMESPACE::engine::impl::Waiter> {
  static constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(USERVER_NAMESPACE::engine::impl::Waiter waiter,
              FormatContext& ctx) const {
    return fmt::format_to(
        ctx.out(), "({}, {})", static_cast<void*>(waiter.context),
        USERVER_NAMESPACE::utils::UnderlyingValue(waiter.epoch));
  }
};
