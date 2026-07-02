#include <engine/impl/async_flat_combining_queue.hpp>

#include <functional>

#include <engine/task/task_context.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

// The consumer is not allowed to leave before being notified, thus this is only a WeakAwaitable.
template <auto TryStartWaiting>
class AsyncFlatCombiningQueue::Awaitable final : public impl::WeakAwaitable {
public:
    explicit Awaitable(AsyncFlatCombiningQueue& queue)
        : queue_(queue)
    {}

    bool IsReady() const noexcept override { return false; }

    void TryAppendAwaiter(boost::intrusive_ptr<Awaiter>& awaiter, [[maybe_unused]] std::uintptr_t context) override {
        if (std::invoke(TryStartWaiting, queue_)) {
            // We committed to sleeping and will be woken up only via
            // NotifyAsyncConsumer. No deadlines or cancellations are allowed,
            // otherwise another thread or task may notify us later and wake up
            // a dead task.
            awaiter = nullptr;
        }
    }

    boost::intrusive_ptr<Awaiter> RemoveAwaiter(
        [[maybe_unused]] Awaiter& awaiter,
        [[maybe_unused]] std::uintptr_t context
    ) noexcept override {
        // The notification happened through consuming_task_context_, but we'll pretend that the awaitable did it.
        // We won't be notified anymore, since we are the sole consumer now.
        return nullptr;
    }

private:
    AsyncFlatCombiningQueue& queue_;
};

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
    UASSERT(!has_waiter_);
}

auto AsyncFlatCombiningQueue::WaitAndStartConsuming() -> Consumer {
    Wait<&AsyncFlatCombiningQueue::TryStartWaitingForConsumer>();
    // The async consumer role is acquired; it is now working.
    notification_state_->store(NotificationState::kWorking);
    return Consumer{*this};
}

void AsyncFlatCombiningQueue::WaitWhileEmpty(Consumer& consumer) noexcept {
    UASSERT(consumer.queue_ == this);
    Wait<&AsyncFlatCombiningQueue::TryStartWaitingWhileEmpty>();
    // Either we were woken up after sleeping, or we were already notified and did
    // not sleep. In both cases we are working again.
    notification_state_->store(NotificationState::kWorking);
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
        notification_state_->store(NotificationState::kWorkingNotified);
        NotifyAsyncConsumer();
        return true;
    } else if (queue_.PushIfEmpty(consumer_node_)) {
        // There is no async consumer anymore.
        notification_state_->store(NotificationState::kWorkingNotified);
        return true;
    } else {
        UASSERT(!has_consumer_.exchange(true));
        return false;
    }
}

void AsyncFlatCombiningQueue::NotifyAsyncConsumer() noexcept {
    UASSERT(consuming_task_context_);
    TaskContext::Wakeup(
        boost::intrusive_ptr<TaskContext>{consuming_task_context_},
        TaskContext::WakeupSource::kNotify,
        NoEpoch{}
    );
}

void AsyncFlatCombiningQueue::NotifyAsyncConsumerIfSleeping() noexcept {
    // If the consumer is sleeping, this wins the race against its CAS to
    // kSleeping and we wake it up. If it is (already) working, it will re-check
    // the queue before sleeping again, so no wakeup is needed.
    if (notification_state_->exchange(NotificationState::kWorkingNotified) == NotificationState::kSleeping) {
        NotifyAsyncConsumer();
    }
}

template <auto TryStartWaiting>
void AsyncFlatCombiningQueue::Wait() noexcept {
    UASSERT(!has_waiter_.exchange(true));
    auto& current = engine::current_task::GetCurrentTaskContext();
    // Check before writing to avoid excessive CPU cache invalidation.
    if (consuming_task_context_ != &current) {
        consuming_task_context_ = &current;
    }
    // No deadlines or cancellations are allowed, otherwise this task may walk away and be destroyed,
    // and the notification will be sent to a dead task.
    UASSERT(!current.IsCancellable());

    Awaitable<TryStartWaiting> awaitable{*this};
    [[maybe_unused]] const auto wakeup_source = current.Sleep(awaitable, Deadline{});
    UASSERT(wakeup_source == TaskContext::WakeupSource::kNotify);

    UASSERT(consuming_task_context_ == &current);
    UASSERT(has_waiter_.exchange(false));
}

bool AsyncFlatCombiningQueue::TryStartWaitingWhileEmpty() {
    // Returns whether we are going to sleep. If a notification arrived while we
    // were working (state is kWorkingNotified), the CAS fails and we keep
    // working, re-checking the queue without sleeping.
    auto expected = NotificationState::kWorking;
    return notification_state_->compare_exchange_strong(expected, NotificationState::kSleeping);
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
