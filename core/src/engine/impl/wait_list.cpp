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
  struct Waiter final {
    boost::intrusive_ptr<TaskContext> context;
    SleepState::Epoch epoch;
  };

  using WaiterQueue = concurrent::impl::ResettableQueue<Waiter>;
  using Handle = WaiterQueue::ItemHandle;

  Impl() = default;

  ~Impl() {
    Waiter waiter;
    if (waiters_.TryPop(waiter)) {
      utils::impl::AbortWithStacktrace(
          "A TaskContext is sleeping in a WaitList that is being destroyed");
    }
  }

  bool WakeupOne() noexcept {
    Waiter waiter;
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
    waiter.context.reset(&context, /*add_ref=*/true);
    waiter.epoch = context.GetEpoch();
    return waiters_.Push(std::move(waiter));
  }

  void Remove(Handle&& handle) noexcept {
    Waiter waiter;
    waiters_.Remove(std::move(handle), waiter);
  }

 private:
  WaiterQueue waiters_;
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
  WaitList::Impl::Handle handle;
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
