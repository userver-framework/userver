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

WaitListLight::WaitListLight() noexcept = default;

WaitListLight::~WaitListLight() {
  UASSERT_MSG(impl_->waiter.IsEmptyRelaxed(),
              "Someone is waiting on WaitListLight while it's being destroyed");
}

void WaitListLight::Append(boost::intrusive_ptr<TaskContext> context) noexcept {
  UASSERT(context);
  UASSERT(context->IsCurrent());

  const std::uintptr_t epoch{utils::UnderlyingValue(context->GetEpoch())};
  LOG_TRACE() << "Append context= " << &context
              << " use_count=" << context->use_count() << " epoch=" << epoch;

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
  LOG_TRACE() << "WakeupOne context=" << &context
              << " use_count=" << context->use_count()
              << " epoch=" << waiter.epoch;
  context->Wakeup(TaskContext::WakeupSource::kWaitList, epoch);
}

void WaitListLight::Remove(TaskContext& context) noexcept {
  UASSERT(context.IsCurrent());

  const std::uintptr_t epoch{utils::UnderlyingValue(context.GetEpoch())};
  if (!impl_->waiter.ResetIfEquals({&context, epoch})) return;

  LOG_TRACE() << "Remove context=" << &context
              << " use_count=" << context.use_count() << " epoch=" << epoch;
  intrusive_ptr_release(&context);
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
