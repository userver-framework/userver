#include "wait_list_light.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <fmt/format.h>
#include <boost/atomic/atomic.hpp>

#include <engine/impl/waiter.hpp>
#include <engine/task/sleep_state.hpp>
#include <engine/task/task_context.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/underlying_value.hpp>
#include <utils/impl/assert_extra.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

struct WaitListLight::Impl final {
  // We use boost::atomic, because std::atomic refuses to produce double-width
  // compare-and-swap instruction (DWCAS) under x86_64 on some compilers.
  // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80878
  // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=84522
  boost::atomic<Waiter> waiter{Waiter{}};
};

WaitListLight::WaitListLight() noexcept = default;

WaitListLight::~WaitListLight() {
  UASSERT_MSG(GetWaiterRelaxed() == nullptr,
              "Someone is waiting on WaitListLight while it's being destroyed");
}

void WaitListLight::WakeupOne() {
  Waiter old_waiter{};
  // We have to use 'compare_exchange_strong' instead of 'load', because old
  // Boost.Atomic has buggy 'load' for x86_64.
  const bool success1 = impl_->waiter.compare_exchange_weak(
      old_waiter, Waiter{}, boost::memory_order_acquire,
      boost::memory_order_acquire);
  if (success1) return;
  UASSERT(old_waiter.context);

  // seq_cst is important for the "Append-Check-Wakeup" sequence.
  const bool success2 = impl_->waiter.compare_exchange_strong(
      old_waiter, Waiter{}, boost::memory_order_seq_cst,
      boost::memory_order_relaxed);
  if (!success2) {
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
  // We have to use 'compare_exchange_strong' instead of 'load', because old
  // Boost.Atomic has buggy 'load' for x86_64.
  Waiter expected{};
  impl_->waiter.compare_exchange_weak(expected, expected,
                                        boost::memory_order_relaxed,
                                        boost::memory_order_relaxed);
  return expected.context;
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

  boost::intrusive_ptr<TaskContext> context(&context_);

  Waiter expected{};
  // seq_cst is important for the "Append-Check-Wakeup" sequence.
  const bool success = owner_.impl_->waiter.compare_exchange_strong(
      expected, new_waiter, boost::memory_order_seq_cst,
      boost::memory_order_relaxed);
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

void WaitScopeLight::Remove() noexcept {
  UASSERT(context_.IsCurrent());
  const Waiter expected{&context_, context_.GetEpoch()};

  auto old_waiter = expected;
  const bool success = owner_.impl_->waiter.compare_exchange_strong(
      old_waiter, Waiter{}, boost::memory_order_release,
      boost::memory_order_relaxed);

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
