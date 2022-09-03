#pragma once

#include <cstdint>
#include <type_traits>

#include <fmt/format.h>

#include <engine/task/sleep_state.hpp>
#include <userver/utils/underlying_value.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskContext;

struct alignas(8) Waiter32 final {
  TaskContext* context{nullptr};
  SleepState::Epoch epoch{0};

  static Waiter32 MakeInvalid(std::size_t invalid_id) noexcept {
    return {reinterpret_cast<TaskContext*>(invalid_id), {}};
  }
};

struct alignas(16) Waiter64 final {
  TaskContext* context{nullptr};
  SleepState::Epoch epoch{0};
  [[maybe_unused]] std::uint32_t padding_dont_use{0};

  static Waiter64 MakeInvalid(std::size_t invalid_id) noexcept {
    return {reinterpret_cast<TaskContext*>(invalid_id), {}, {}};
  }
};

using Waiter = std::conditional_t<sizeof(void*) == 8, Waiter64, Waiter32>;

inline bool operator==(Waiter lhs, Waiter rhs) noexcept {
  return lhs.context == rhs.context && lhs.epoch == rhs.epoch;
}

}  // namespace engine::impl

USERVER_NAMESPACE_END

template <>
struct fmt::formatter<USERVER_NAMESPACE::engine::impl::Waiter> {
  static constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(USERVER_NAMESPACE::engine::impl::Waiter waiter,
              FormatContext& ctx) const {
    return fmt::format_to(
        ctx.out(), "({}, {})", fmt::ptr(waiter.context),
        USERVER_NAMESPACE::utils::UnderlyingValue(waiter.epoch));
  }
};
