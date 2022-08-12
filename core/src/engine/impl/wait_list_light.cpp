#include "wait_list_light.hpp"

#include <cstddef>

#include <fmt/format.h>

#include <engine/impl/atomic_waiter.hpp>
#include <engine/task/task_context.hpp>
#include <userver/utils/assert.hpp>

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

  const Waiter waiter{context.get(), context->GetEpoch()};
  LOG_TRACE() << "Append waiter=" << fmt::to_string(waiter)
              << " use_count=" << context->use_count();

  // Keep a reference logically stored in the WaitListLight to ensure that
  // WakeupOne can complete safely in parallel with the waiting task being
  // cancelled, Remove-d and stopped.
  context.detach();

  impl_->waiter.Set(waiter);
}

void WaitListLight::WakeupOne() {
  const auto waiter = impl_->waiter.GetAndReset();
  const boost::intrusive_ptr<TaskContext> context{waiter.context,
                                                  /*add_ref=*/false};
  if (!context) return;

  LOG_TRACE() << "WakeupOne waiter=" << fmt::to_string(waiter)
              << " use_count=" << context->use_count();
  context->Wakeup(TaskContext::WakeupSource::kWaitList, waiter.epoch);
}

void WaitListLight::Remove(TaskContext& context) noexcept {
  UASSERT(context.IsCurrent());
  const Waiter waiter{&context, context.GetEpoch()};
  if (!impl_->waiter.ResetIfEquals(waiter)) return;

  LOG_TRACE() << "Remove waiter=" << fmt::to_string(waiter)
              << " use_count=" << context.use_count();
  intrusive_ptr_release(&context);
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
