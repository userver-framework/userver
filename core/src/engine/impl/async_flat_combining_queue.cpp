#include <engine/impl/async_flat_combining_queue.hpp>

#include <functional>

#include <engine/task/task_context.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

template <auto TryStartWaiting>
class AsyncFlatCombiningQueue::WaitStrategy final : public impl::WaitStrategy {
public:
    WaitStrategy(AsyncFlatCombiningQueue& queue, TaskContext& context) : queue_(queue), context_(context) {
        // No deadlines or cancellations are allowed, because else this task may
        // walk away and be destroyed, and the notification will be sent to a dead
        // task.
        UASSERT(!context_.IsCancellable());
    }

    EarlyWakeup SetupWakeups() override {
        if (std::invoke(TryStartWaiting, queue_)) {
            // We will be woken up if and only if our notifier_node_ is seen by
            // another thread or task. No deadlines or cancellations are allowed,
            // otherwise another consumer may see notifier_node_ later and wake up
            // a dead task.
            return EarlyWakeup{false};
        } else {
            return EarlyWakeup{true};
        }
    }

    void DisableWakeups() noexcept override {
        // We won't be notified anymore, since we are the sole consumer now.
    }

private:
    AsyncFlatCombiningQueue& queue_;
    TaskContext& context_;
};

AsyncFlatCombiningQueue::Consumer::Consumer(AsyncFlatCombiningQueue& queue) : queue_(&queue) {
    UASSERT(!queue_->has_consumer_.exchange(true));
}

AsyncFlatCombiningQueue::Consumer::Consumer(Consumer&& other) noexcept : queue_(std::exchange(other.queue_, nullptr)) {}

auto AsyncFlatCombiningQueue::Consumer::operator=(Consumer&& other) noexcept -> Consumer& {
    queue_ = std::exchange(other.queue_, nullptr);
    return *this;
}

AsyncFlatCombiningQueue::Consumer::~Consumer() {
    UASSERT_MSG(!queue_, "Consumer must process all nodes before leaving");
}

auto AsyncFlatCombiningQueue::Consumer::TryPop() noexcept -> NodeBase* {
    UASSERT(queue_);
    return queue_->DoTryPop();
}

bool AsyncFlatCombiningQueue::Consumer::TryStopConsuming() noexcept {
    UASSERT(queue_);
    const bool has_left = queue_->DoTryStopConsuming();
    if (has_left) {
        queue_ = nullptr;
    }
    return has_left;
}

AsyncFlatCombiningQueue::AsyncFlatCombiningQueue() { queue_.Push(consumer_node_); }

AsyncFlatCombiningQueue::~AsyncFlatCombiningQueue() {
    UASSERT(!DoTryPop());
    UASSERT(!has_consumer_);
    UASSERT(!has_waiter_);
}

auto AsyncFlatCombiningQueue::WaitAndStartConsuming() -> Consumer {
    Wait<&AsyncFlatCombiningQueue::TryStartWaitingForConsumer>();
    return Consumer{*this};
}

void AsyncFlatCombiningQueue::WaitWhileEmpty(Consumer& consumer) noexcept {
    UASSERT(consumer.queue_ == this);
    Wait<&AsyncFlatCombiningQueue::TryStartWaitingWhileEmpty>();
    if (std::exchange(should_pop_notifier_node_, false)) {
        [[maybe_unused]] const auto* const node = queue_.TryPopBlocking();
        UASSERT(node == &while_empty_notifier_node_);
    }
}

AsyncFlatCombiningQueue::NodeBase* AsyncFlatCombiningQueue::DoTryPop() noexcept {
    while (true) {
        auto* const node = queue_.TryPopBlocking();
        if (!node) return nullptr;

        if (node == &start_consuming_notifier_node_) {
            // Another task is waiting in WaitAndStartConsuming.
            should_pass_consumer_to_waiter_ = true;
            // The waiter will consume the remaining nodes.
            return nullptr;
        }
        if (node == &consumer_node_) {
            continue;
        }

        return node;
    }
}

bool AsyncFlatCombiningQueue::DoTryStopConsuming() noexcept {
    // We have to temporarily relax the check, because on success, another
    // consumer may arrive even before queue_->TryStopConsuming finishes.
    UASSERT(has_consumer_.exchange(false));
    if (std::exchange(should_pass_consumer_to_waiter_, false)) {
        // The waiter will consume the remaining nodes.
        NotifyAsyncConsumer();
        return true;
    } else if (queue_.PushIfEmpty(consumer_node_)) {
        return true;
    } else {
        UASSERT(!has_consumer_.exchange(true));
        return false;
    }
}

void AsyncFlatCombiningQueue::NotifyAsyncConsumer() noexcept {
    UASSERT(consuming_task_context_);
    consuming_task_context_->Wakeup(TaskContext::WakeupSource::kWaitList, TaskContext::NoEpoch{});
}

template <auto TryStartWaiting>
void AsyncFlatCombiningQueue::Wait() noexcept {
    UASSERT(!has_waiter_.exchange(true));
    auto& current = engine::current_task::GetCurrentTaskContext();
    // Check before writing to avoid excessive CPU cache invalidation.
    if (consuming_task_context_ != &current) consuming_task_context_ = &current;

    WaitStrategy<TryStartWaiting> wait_strategy{*this, current};
    [[maybe_unused]] const auto wakeup_source = current.Sleep(wait_strategy, Deadline{});
    UASSERT(wakeup_source == TaskContext::WakeupSource::kWaitList);

    UASSERT(consuming_task_context_ == &current);
    UASSERT(has_waiter_.exchange(false));
}

bool AsyncFlatCombiningQueue::TryStartWaitingWhileEmpty() {
    UASSERT_MSG(!should_pop_notifier_node_, "Multiple consumers detected");
    const bool pushed = queue_.PushIfEmpty(while_empty_notifier_node_);
    should_pop_notifier_node_ = pushed;
    return pushed;
}

bool AsyncFlatCombiningQueue::TryStartWaitingForConsumer() {
    const auto* const prev = queue_.GetBackAndPush(start_consuming_notifier_node_);
    UASSERT(prev != &while_empty_notifier_node_);
    UASSERT(prev != &start_consuming_notifier_node_);

    if (prev == &consumer_node_) {
        // We are the consumer now.
        // Retrieve notifier_node_ to avoid a spurious wakeup later.
        [[maybe_unused]] const auto* const node1 = queue_.TryPopBlocking();
        UASSERT(node1 == &consumer_node_);
        [[maybe_unused]] const auto* const node2 = queue_.TryPopBlocking();
        UASSERT(node2 == &start_consuming_notifier_node_);
        return false;  // wakeup self
    } else {
        return true;
    }
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
