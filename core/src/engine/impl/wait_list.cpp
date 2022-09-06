#include "wait_list.hpp"

#include <atomic>

#include <concurrent/resettable_queue.hpp>
#include <engine/impl/waiter.hpp>
#include <engine/task/task_context.hpp>
#include <utils/impl/assert_extra.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class WaitList::Impl final {
 public:
  using Handle = concurrent::impl::ResettableQueue<Waiter>::ItemHandle;

  Impl() = default;

  ~Impl() {
    Waiter waiter;
    if (waiters_.TryPop(waiter)) {
      utils::impl::AbortWithStacktrace(
          "A TaskContext is sleeping in a WaitList that is being destroyed");
    }
  }

  bool WakeupOne() noexcept {
    Waiter waiter{nullptr};
    while (waiters_.TryPop(waiter)) {
      if (waiter.context->Wakeup(TaskContext::WakeupSource::kWaitList,
                                 waiter.epoch)) {
        return true;
      }
    }
    return false;
  }

  Handle Append(TaskContext& context) {
    Waiter waiter;
    waiter.context = &context;
    waiter.epoch = context.GetEpoch();
    // TODO An out-of-memory exception can theoretically fly from there.
    //  Preallocate queue nodes?
    return waiters_.Push(waiter);
  }

  void Remove(Handle&& handle) noexcept { waiters_.Remove(std::move(handle)); }

 private:
  concurrent::impl::ResettableQueue<Waiter> waiters_;
};

WaitList::WaitList() = default;

WaitList::~WaitList() = default;

void WaitList::WakeupOne() noexcept { impl_->WakeupOne(); }

void WaitList::WakeupAll() noexcept {
  while (impl_->WakeupOne()) {
  }
}

struct WaitScope::Impl final {
  WaitList& owner;
  TaskContext& context;
  WaitList::Impl::Handle handle{};
};

WaitScope::WaitScope(WaitList& owner, TaskContext& context)
    : impl_(Impl{owner, context, {}}) {}

WaitScope::~WaitScope() = default;

TaskContext& WaitScope::GetContext() const noexcept { return impl_->context; }

void WaitScope::Append() noexcept {
  impl_->handle = impl_->owner.impl_->Append(impl_->context);
}

void WaitScope::Remove() noexcept {
  impl_->owner.impl_->Remove(std::move(impl_->handle));
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
