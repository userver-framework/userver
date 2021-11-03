#include "wait_list_light.hpp"

#include <engine/task/task_context.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

#ifndef NDEBUG
WaitListLight::SingleUserGuard::SingleUserGuard(WaitListLight& wait_list)
    : wait_list_(wait_list) {
  auto& task = engine::current_task::GetCurrentTaskContext();
  UASSERT(!wait_list_.owner_);
  wait_list_.owner_ = &task;
}

WaitListLight::SingleUserGuard::~SingleUserGuard() {
  auto& task = engine::current_task::GetCurrentTaskContext();
  UASSERT(wait_list_.owner_ == &task);
  wait_list_.owner_ = nullptr;
}
#endif

WaitListLight::~WaitListLight() = default;

bool WaitListLight::IsEmpty() const { return waiting_; }

void WaitListLight::Append(
    boost::intrusive_ptr<impl::TaskContext> context) noexcept {
  UASSERT(context);
  UASSERT(context->IsCurrent());
  UASSERT(!waiting_);
#ifndef NDEBUG
  UASSERT(!owner_ || owner_ == context.get());
#endif

  LOG_TRACE() << "Appending, use_count=" << context->use_count();
  waiting_ = context.detach();
}

void WaitListLight::WakeupOne() {
  ++in_wakeup_;
  utils::ScopeGuard guard([this] { --in_wakeup_; });

  auto old = waiting_.exchange(nullptr);
  if (old) {
    LOG_TRACE() << "Waking up! use_count=" << old->use_count();
    old->Wakeup(impl::TaskContext::WakeupSource::kWaitList,
                impl::TaskContext::NoEpoch{});
    intrusive_ptr_release(old);
  }
}

void WaitListLight::Remove(impl::TaskContext& context) noexcept {
  LOG_TRACE() << "remove (cancel)";
  auto old = waiting_.exchange(nullptr);

  UASSERT(!old || old == &context);

  if (old) {
    intrusive_ptr_release(old);
  } else {
    /*
     * Race with WakeupOne()/WakeupAll() has fired. We have to wait until
     * Wakeup*() in another thread has finished, otherwise Wakeup*() might awake
     * the task too late (e.g. after the next Sleep()) which results in a
     * spurious wakeup (very bad, not all sync primitives check for spurious
     * wakeups, e.g. SleepFor()).
     */

    LOG_TRACE() << "Race with Wakeup*(), spinning";
    while (in_wakeup_) {
      std::this_thread::yield();
    }
  }
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
