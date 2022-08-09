#include "wait_list_light.hpp"

#include <cstddef>

#include <engine/impl/atomic_waiter.hpp>
#include <engine/task/task_context.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/underlying_value.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

struct WaitListLight::Impl final {
  AtomicWaiter waiter;
};

WaitListLight::WaitListLight() noexcept : impl_() {}

WaitListLight::~WaitListLight() {
  UASSERT_MSG(impl_->waiter.IsEmpty(),
              "Someone is waiting on WaitListLight while it's being destroyed");
}

void WaitListLight::Append(boost::intrusive_ptr<TaskContext> context) noexcept {
  UASSERT(context);
  UASSERT(context->IsCurrent());
  UASSERT_MSG(impl_->waiter.IsEmpty(),
              "Another coroutine is already waiting on this WaitListLight");
  LOG_TRACE() << "Appending, use_count=" << context->use_count();

  const std::uintptr_t epoch{utils::UnderlyingValue(context->GetEpoch())};

  // Keep a reference logically stored in the WaitListLight to ensure that
  // WakeupOne can complete safely in parallel with the waiting task being
  // cancelled, Remove-d and stopped.
  auto* const ctx = context.detach();

  impl_->waiter.Set({ctx, epoch});
}

void WaitListLight::WakeupOne() {
  const auto waiter = impl_->waiter.GetAndReset();
  const boost::intrusive_ptr<TaskContext> context{waiter.context,
                                                  /*add_ref=*/false};
  if (!context) return;

  const auto epoch = static_cast<SleepState::Epoch>(waiter.epoch);

  LOG_TRACE() << "Waking up, use_count=" << context->use_count();
  context->Wakeup(TaskContext::WakeupSource::kWaitList, epoch);
}

void WaitListLight::Remove(TaskContext& context) noexcept {
  UASSERT(context.IsCurrent());
  const auto waiter = impl_->waiter.GetAndReset();
  if (!waiter.context) return;

  UASSERT(waiter.context == &context);
  LOG_TRACE() << "Remove, use_count=" << context.use_count();
  intrusive_ptr_release(&context);
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
