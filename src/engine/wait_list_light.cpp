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
  LOG_TRACE() << "Appending";
  assert(!waiting_);
  assert(owner_ == ctx.get());
  waiting_ = ctx.get();
  ctx.detach();
}

void WaitListLight::WakeupOne(WaitListBase::Lock&) {
  LOG_TRACE() << "WakeupOne";
  auto old = waiting_.load();
  if (old) {
    LOG_TRACE() << "Waking up!";
    old->Wakeup(impl::TaskContext::WakeupSource::kWaitList);
  }
}

void WaitListLight::WakeupAll(WaitListBase::Lock& lock) { WakeupOne(lock); }

void WaitListLight::Remove(impl::TaskContext& ctx) {
  LOG_TRACE() << "remove (ref)";
  auto old = waiting_.exchange(nullptr);
  assert(old == &ctx);
  boost::ignore_unused(old);
  boost::intrusive_ptr<impl::TaskContext> old_ctx(&ctx, false);
}

void WaitListLight::Remove(const boost::intrusive_ptr<impl::TaskContext>& ctx) {
  LOG_TRACE() << "remove (cancel)";
  assert(waiting_.load() == ctx.get());
  boost::ignore_unused(ctx);
  boost::intrusive_ptr<impl::TaskContext> old_ctx(ctx.get(), false);
}

}  // namespace impl
}  // namespace engine
