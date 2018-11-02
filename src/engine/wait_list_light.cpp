#include "wait_list_light.hpp"

#include <boost/core/ignore_unused.hpp>

#include <engine/task/task_context.hpp>

namespace engine {
namespace impl {

WaitListLight::Lock::operator bool() {
  assert(false && "must not be called");
  return true;
}

WaitListLight::~WaitListLight() = default;

void WaitListLight::PinToCurrentTask() {
#ifndef NDEBUG
  auto task = engine::current_task::GetCurrentTaskContext();
  assert(task);
  PinToTask(*task);
#endif
}

void WaitListLight::PinToTask(impl::TaskContext& ctx) {
  assert(!owner_ || owner_ == &ctx);
#ifndef NDEBUG
  owner_ = &ctx;
#endif
  boost::ignore_unused(ctx);
}

void WaitListLight::Append(WaitListBase::Lock&,
                           boost::intrusive_ptr<impl::TaskContext> ctx) {
  LOG_TRACE() << "Appending, use_count=" << ctx->use_count();
  assert(!waiting_);
  assert(owner_ == ctx.get());

  auto ptr = ctx.get();
  intrusive_ptr_add_ref(ptr);
  waiting_ = ptr;
}

void WaitListLight::WakeupOne(WaitListBase::Lock&) {
  LOG_TRACE() << "WakeupOne";
  auto old = waiting_.exchange(nullptr);
  if (old) {
    LOG_TRACE() << "Waking up! use_count=" << old->use_count();
    old->Wakeup(impl::TaskContext::WakeupSource::kWaitList);
    intrusive_ptr_release(old);
  }
}

void WaitListLight::WakeupAll(WaitListBase::Lock& lock) { WakeupOne(lock); }

void WaitListLight::Remove(const boost::intrusive_ptr<impl::TaskContext>& ctx) {
  LOG_TRACE() << "remove (cancel)";
  auto old = waiting_.exchange(nullptr);

  assert(!old || old == ctx.get());
  boost::ignore_unused(ctx);

  if (old) intrusive_ptr_release(old);
}

}  // namespace impl
}  // namespace engine
