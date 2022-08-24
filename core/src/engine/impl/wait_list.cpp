#include "wait_list.hpp"

#include <atomic>
#include <cstddef>

#include <moodycamel/concurrentqueue.h>

#include <concurrent/intrusive_walkable_pool.hpp>
#include <engine/impl/wait_list_light.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

namespace {

bool ShouldCleanup(std::size_t node_count, std::size_t garbage_count) noexcept {
  return garbage_count >= 32 && garbage_count >= 2 * node_count;
}

}  // namespace

struct WaitList::WaiterNode final {
  WaitListLight waiter;
};

class WaitList::Impl final {
 public:
  Impl()
      : waiters_(std::make_unique<moodycamel::ConcurrentQueue<WaiterNode*>>(0)),
        free_list_(
            std::make_unique<moodycamel::ConcurrentQueue<WaiterNode*>>(0)) {}

  ~Impl() {
    WaiterNode* node{};
    while (free_list_->try_dequeue(node)) {
      UASSERT(node);
      delete node;
    }
  }

  bool WakeupOne() {
    WaiterNode* waiter{nullptr};
    while (waiters_->try_dequeue(waiter)) {
      UASSERT(waiter);
      if (waiter->waiter.WakeupOne()) return true;
    }
    return false;
  }

  WaiterNode& AcquireWaiterNode() {
    WaiterNode* node{};
    if (free_list_->try_dequeue(node)) {
      return *node;
    }
    node_count_.fetch_add(1, std::memory_order_relaxed);
    return *new WaiterNode{};
  }

  void ReleaseWaiterNode(WaiterNode& node) noexcept {
    free_list_->enqueue(&node);
  }

  void AppendActiveWaiter(WaiterNode& node) noexcept {
    waiters_->enqueue(&node);
  }

  void AccountGarbageNode() noexcept {
    garbage_count_approx_.fetch_add(1, std::memory_order_release);
    const auto node_count = node_count_.load(std::memory_order_relaxed);
    const auto garbage_count_approx =
        garbage_count_approx_.load(std::memory_order_acquire);

    if (ShouldCleanup(node_count, garbage_count_approx)) {
      const auto garbage_count =
          garbage_count_approx_.exchange(0, std::memory_order_acq_rel);
      if (ShouldCleanup(node_count, garbage_count)) {
        // Yeah, that's a bunch of spurious wakeups.
        while (WakeupOne()) {
        }
      } else {
        garbage_count_approx_.fetch_add(0, std::memory_order_release);
      }
    }
  }

 private:
  std::unique_ptr<moodycamel::ConcurrentQueue<WaiterNode*>> waiters_;
  std::unique_ptr<moodycamel::ConcurrentQueue<WaiterNode*>> free_list_;
  std::atomic<std::size_t> node_count_{0};
  std::atomic<std::size_t> garbage_count_approx_{0};
};

WaitList::WaitList() = default;

WaitList::~WaitList() = default;

void WaitList::WakeupOne() noexcept { impl_->WakeupOne(); }

void WaitList::WakeupAll() noexcept {
  while (impl_->WakeupOne()) {
  }
}

WaitScope::WaitScope(WaitList& owner, TaskContext& context)
    : owner_(owner),
      node_(owner_.impl_->AcquireWaiterNode()),
      scope_light_(node_.waiter, context) {}

WaitScope::~WaitScope() { owner_.impl_->ReleaseWaiterNode(node_); }

TaskContext& WaitScope::GetContext() const noexcept {
  return scope_light_.GetContext();
}

void WaitScope::Append() noexcept {
  scope_light_.Append();
  owner_.impl_->AppendActiveWaiter(node_);
}

void WaitScope::Remove() noexcept {
  const bool not_woken_up = scope_light_.Remove();
  if (not_woken_up) {
    owner_.impl_->AccountGarbageNode();
  }
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
