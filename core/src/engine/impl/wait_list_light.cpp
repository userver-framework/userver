#include "wait_list_light.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <fmt/format.h>

#include <concurrent/impl/fast_atomic.hpp>
#include <engine/impl/waiter.hpp>
#include <engine/task/sleep_state.hpp>
#include <engine/task/task_context.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/underlying_value.hpp>
#include <utils/impl/assert_extra.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

struct WaitListLight::Impl final {
  concurrent::impl::FastAtomic<Waiter> waiter{Waiter{}};
};

WaitListLight::WaitListLight() noexcept = default;

WaitListLight::~WaitListLight() {
  UASSERT_MSG(GetWaiterRelaxed() == nullptr,
              "Someone is waiting on WaitListLight while it's being destroyed");
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
              << " use_count=" << context->use_count();
  context->Wakeup(TaskContext::WakeupSource::kWaitList, old_waiter.epoch);
}

TaskContext* WaitListLight::GetWaiterRelaxed() noexcept {
  return impl_->waiter.load(std::memory_order_relaxed).context;
}

WaitScopeLight::WaitScopeLight(WaitListLight& owner, TaskContext& context)
    : owner_(owner), context_(context) {}

WaitScopeLight::~WaitScopeLight() {
  UASSERT_MSG(owner_.GetWaiterRelaxed() != &context_,
              "Append was called without a subsequent Remove");
}

void WaitScopeLight::Append() noexcept {
  UASSERT(context_.IsCurrent());

  const Waiter new_waiter{&context_, context_.GetEpoch()};
  LOG_TRACE() << "Append waiter=" << fmt::to_string(new_waiter)
              << " use_count=" << context_.use_count();

  boost::intrusive_ptr context_ptr(&context_, /*add_ref=*/true);

  Waiter expected{};
  // seq_cst is important for the "Append-Check-Wakeup" sequence.
  const bool success = owner_.impl_->waiter.compare_exchange_strong(
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
  context_ptr.detach();
}

void WaitScopeLight::Remove() noexcept {
  UASSERT(context_.IsCurrent());
  const Waiter expected{&context_, context_.GetEpoch()};

  auto old_waiter = expected;
  const bool success = owner_.impl_->waiter.compare_exchange_strong(
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
              << " use_count=" << context_.use_count();
  intrusive_ptr_release(&context_);
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
