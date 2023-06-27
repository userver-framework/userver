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
  std::uint32_t padding_dont_use{0};
};

// Silence 'unused' warning.
static_assert(sizeof(Waiter64::padding_dont_use) != 0);

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
namespace {

TaskContext* const kSignaled = reinterpret_cast<TaskContext*>(1);

void DoWakeup(Waiter waiter) {
  LOG_TRACE() << "WakeupOne waiter=" << fmt::to_string(waiter)
              << " use_count=" << waiter.context->UseCount();
  waiter.context->Wakeup(TaskContext::WakeupSource::kWaitList, waiter.epoch);
}

}  // namespace

struct WaitListLight::Impl final {
  concurrent::impl::FastAtomic<Waiter> waiter{Waiter{}};
};

WaitListLight::WaitListLight() noexcept = default;

WaitListLight::~WaitListLight() {
  UASSERT_MSG(IsEmptyRelaxed(),
              "Someone is waiting on WaitListLight while it's being destroyed");
}

void WaitListLight::Append(
    boost::intrusive_ptr<impl::TaskContext>&& context) noexcept {
  [[maybe_unused]] const bool was_signaled =
      GetSignalOrAppend(std::move(context));
  UASSERT_MSG(!was_signaled, "Signals cannot be used with plain Append");
}

bool WaitListLight::GetSignalOrAppend(
    boost::intrusive_ptr<TaskContext>&& context) noexcept {
  UASSERT(context);
  UASSERT(context->IsCurrent());

  const Waiter new_waiter{context.get(), context->GetEpoch()};
  LOG_TRACE() << "Append waiter=" << fmt::to_string(new_waiter)
              << " use_count=" << context->UseCount();

  Waiter expected{};
  // seq_cst is important for the "Append-Check-Wakeup" sequence.
  const bool success =
      impl_->waiter.compare_exchange_strong<std::memory_order_seq_cst,
                                            std::memory_order_relaxed>(
          expected, new_waiter);
  if (!success) {
    UASSERT_MSG(expected.context == kSignaled,
                fmt::format("Attempting to wait in a single AtomicWaiter "
                            "from multiple coroutines: new={} existing={}",
                            new_waiter, expected));
    return true;
  }

  // Keep a reference logically stored in the WaitListLight to ensure that
  // WakeupOne can complete safely in parallel with the waiting task being
  // cancelled, Remove-d and stopped.
  context.detach();

  return false;
}

void WaitListLight::WakeupOne() {
  // seq_cst is important for the "Append-Check-Wakeup" sequence.
  const auto old_waiter = impl_->waiter.load<std::memory_order_seq_cst>();
  if (old_waiter.context == nullptr) return;

  UASSERT_MSG(old_waiter.context != kSignaled,
              "Use SetSignalAndWakeupOne for dealing with signals instead");
  DoWakeup(old_waiter);
  // The waiter will Remove itself later.
}

void WaitListLight::SetSignalAndWakeupOne() {
  // seq_cst is important for the "Append-Check-Wakeup" sequence.
  const auto old_waiter =
      impl_->waiter.exchange<std::memory_order_seq_cst>(Waiter{kSignaled, {}});
  if (old_waiter.context == nullptr || old_waiter.context == kSignaled) {
    return;
  }

  const boost::intrusive_ptr<TaskContext> context{old_waiter.context,
                                                  /*add_ref=*/false};
  DoWakeup(old_waiter);
}

void WaitListLight::Remove(TaskContext& context) noexcept {
  UASSERT(context.IsCurrent());
  const Waiter expected{&context, context.GetEpoch()};

  auto old_waiter = expected;
  const bool success =
      impl_->waiter.compare_exchange_strong<std::memory_order_release,
                                            std::memory_order_relaxed>(
          old_waiter, Waiter{});

  if (!success) {
    UASSERT_MSG(
        old_waiter.context == nullptr || old_waiter.context == kSignaled,
        fmt::format("An unexpected context is occupying the "
                    "WaitListLight: expected={} actual={}",
                    expected, old_waiter));
    return;
  }

  LOG_TRACE() << "Remove waiter=" << fmt::to_string(expected)
              << " use_count=" << context.UseCount();
  intrusive_ptr_release(&context);
}

bool WaitListLight::GetAndResetSignal() noexcept {
  Waiter expected{kSignaled, {}};
  const bool success =
      impl_->waiter.compare_exchange_strong<std::memory_order_relaxed,
                                            std::memory_order_relaxed>(
          expected, Waiter{});

  if (!success && expected.context != nullptr) {
    utils::impl::AbortWithStacktrace(fmt::format(
        "ResetSignal with an active waiter is not allowed: waiter={}",
        expected));
  }

  return success;
}

bool WaitListLight::IsSignaled() const noexcept {
  return impl_->waiter.load<std::memory_order_seq_cst>().context == kSignaled;
}

bool WaitListLight::IsEmptyRelaxed() noexcept {
  const auto waiter = impl_->waiter.load<std::memory_order_relaxed>();
  return waiter.context == nullptr || waiter.context == kSignaled;
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
