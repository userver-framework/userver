#include <engine/impl/async_flat_combining_queue.hpp>

#include <engine/task/task_context.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

AsyncFlatCombiningQueue::Consumer::Consumer(AsyncFlatCombiningQueue& queue)
    : queue_(&queue)
{
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
}

auto AsyncFlatCombiningQueue::WaitAndStartConsuming() -> Consumer {
    if (TryStartWaitingForConsumer()) {
        // No deadlines or cancellations are allowed here, otherwise this task may
        // walk away, and DoTryStopConsuming will try to hand over the consumer
        // role to nobody.
        UASSERT(!engine::current_task::GetCurrentTaskContext().IsCancellable());
        [[maybe_unused]] const bool success = start_consuming_event_.WaitForEvent();
        UASSERT(success);
    }
    // The async consumer role is acquired.
    return Consumer{*this};
}

void AsyncFlatCombiningQueue::WaitWhileEmpty(Consumer& consumer) noexcept {
    UASSERT(consumer.queue_ == this);
    // No deadlines or cancellations are allowed here, otherwise this task may
    // walk away while still being registered as the queue's consumer.
    UASSERT(!engine::current_task::GetCurrentTaskContext().IsCancellable());
    // Auto-resets on success, guaranteeing an 'acquire' on the signal, so that
    // the newly pushed nodes are visible to the subsequent DoTryPop calls.
    [[maybe_unused]] const bool success = empty_queue_event_->WaitForEvent();
    UASSERT(success);
}

AsyncFlatCombiningQueue::NodeBase* AsyncFlatCombiningQueue::DoTryPop() noexcept {
    while (true) {
        auto* const node = queue_.TryPopBlocking();
        if (!node) {
            return nullptr;
        }

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
        start_consuming_event_.Send();
        return true;
    } else if (queue_.PushIfEmpty(consumer_node_)) {
        // There is no async consumer anymore.
        return true;
    } else {
        UASSERT(!has_consumer_.exchange(true));
        return false;
    }
}

void AsyncFlatCombiningQueue::NotifyAsyncConsumerIfSleeping() noexcept {
    // If the consumer is sleeping, this wakes it up. If it is (already)
    // working, it will re-check the queue before sleeping again, so no wakeup
    // is needed; Send() only sets the signal flag in that case.
    empty_queue_event_->Send();
}

bool AsyncFlatCombiningQueue::TryStartWaitingForConsumer() {
    const auto* const prev = queue_.GetBackAndPush(start_consuming_notifier_node_);
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
