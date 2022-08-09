#include "wait_list.hpp"

#include <concurrent/intrusive_walkable_pool.hpp>
#include <engine/impl/wait_list_light.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

struct WaitList::WaiterNode final {
  WaitListLight waiter;
  concurrent::impl::IntrusiveWalkablePoolHook<WaiterNode> hook;
};

struct WaitList::Impl final {
  using WaitersPool = concurrent::impl::IntrusiveWalkablePool<
      WaiterNode, concurrent::impl::MemberHook<&WaiterNode::hook>>;

  WaitersPool waiters;
};

WaitList::WaitList() = default;

WaitList::~WaitList() = default;

void WaitList::WakeupOne() noexcept {
  impl_->waiters.WalkWhile(
      [&](WaiterNode& node) { return !node.waiter.WakeupOne(); });
}

void WaitList::WakeupAll() noexcept {
  impl_->waiters.Walk([&](WaiterNode& node) { node.waiter.WakeupOne(); });
}

WaitScope::WaitScope(WaitList& owner, TaskContext& context)
    : owner_(owner),
      node_(owner_.impl_->waiters.Acquire()),
      scope_light_(node_.waiter, context) {}

WaitScope::~WaitScope() { owner_.impl_->waiters.Release(node_); }

TaskContext& WaitScope::GetContext() const noexcept {
  return scope_light_.GetContext();
}

void WaitScope::Append() noexcept { return scope_light_.Append(); }

void WaitScope::Remove() noexcept { return scope_light_.Remove(); }

}  // namespace engine::impl

USERVER_NAMESPACE_END
