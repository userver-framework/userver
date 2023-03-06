#include "wait_list_light.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <fmt/format.h>

#include <concurrent/impl/fast_atomic.hpp>
#include <engine/task/sleep_state.hpp>
#include <engine/task/task_context.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/underlying_value.hpp>
#include <utils/impl/assert_extra.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {
namespace {

struct alignas(8) Waiter32 final {
  TaskContext* context{nullptr};
  SleepState::Epoch epoch{0};
};

struct alignas(16) Waiter64 final {
  TaskContext* context{nullptr};
  SleepState::Epoch epoch{0};
  [[maybe_unused]] std::uint32_t padding_dont_use{0};
};

using Waiter = std::conditional_t<sizeof(void*) == 8, Waiter64, Waiter32>;

}  // namespace
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

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

struct WaitListLight::Impl final {
  concurrent::impl::FastAtomic<Waiter> waiter{Waiter{}};
};

WaitListLight::WaitListLight() noexcept = default;

WaitListLight::~WaitListLight() {
  UASSERT_MSG(IsEmptyRelaxed(),
              "Someone is waiting on WaitListLight while it's being destroyed");
}

void WaitListLight::Append(boost::intrusive_ptr<TaskContext> context) noexcept {
  UASSERT(context);
  UASSERT(context->IsCurrent());

  const Waiter new_waiter{context.get(), context->GetEpoch()};
  LOG_TRACE() << "Append waiter=" << fmt::to_string(new_waiter)
              << " use_count=" << context->UseCount();

  Waiter expected{};
  // seq_cst is important for the "Append-Check-Wakeup" sequence.
  const bool success = impl_->waiter.compare_exchange_strong(
      expected, new_waiter, std::memory_order_seq_cst,
      std::memory_order_relaxed);
  if (!success) {
    utils::impl::AbortWithStacktrace(
        fmt::format("Attempting to wait in a single AtomicWaiter "
                    "from multiple coroutines: new={} existing={}",
                    new_waiter, expected));
  }

  // Keep a reference logically stored in the WaitListLight to ensure that
  // WakeupOne can complete safely in parallel with the waiting task being
  // cancelled, Remove-d and stopped.
  context.detach();
}

void WaitListLight::WakeupOne() {
  auto old_waiter = impl_->waiter.load(std::memory_order_acquire);
  if (!old_waiter.context) return;

  // seq_cst is important for the "Append-Check-Wakeup" sequence.
  const bool success = impl_->waiter.compare_exchange_strong(
      old_waiter, Waiter{}, std::memory_order_seq_cst,
      std::memory_order_relaxed);
  if (!success) {
    if (!old_waiter.context) return;
    // The waiter has changed from one non-null value to another non-null value.
    // This means that during this execution of WakeupOne one waiter was
    // Removed, and another one was Appended. Pretend that we were called
    // between Remove and Append.
    return;
  }

  const boost::intrusive_ptr<TaskContext> context{old_waiter.context,
                                                  /*add_ref=*/false};

  LOG_TRACE() << "WakeupOne waiter=" << fmt::to_string(old_waiter)
              << " use_count=" << context->UseCount();
  context->Wakeup(TaskContext::WakeupSource::kWaitList, old_waiter.epoch);
}

void WaitListLight::Remove(TaskContext& context) noexcept {
  UASSERT(context.IsCurrent());
  const Waiter expected{&context, context.GetEpoch()};

  auto old_waiter = expected;
  const bool success = impl_->waiter.compare_exchange_strong(
      old_waiter, Waiter{}, std::memory_order_release,
      std::memory_order_relaxed);

  if (!success) {
    UASSERT_MSG(!old_waiter.context,
                fmt::format("An unexpected context is occupying the "
                            "AtomicWaiter: expected={} actual={}",
                            expected, old_waiter));
    return;
  }

  LOG_TRACE() << "Remove waiter=" << fmt::to_string(expected)
              << " use_count=" << context.UseCount();
  intrusive_ptr_release(&context);
}

bool WaitListLight::IsEmptyRelaxed() noexcept {
  return impl_->waiter.load(std::memory_order_relaxed).context == nullptr;
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
