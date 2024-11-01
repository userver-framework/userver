#pragma once

/// @file userver/concurrent/mpsc_queue.hpp
/// @brief Multiple producer, single consumer queue

#include <atomic>
#include <limits>
#include <memory>

#include <userver/concurrent/impl/intrusive_mpsc_queue.hpp>
#include <userver/concurrent/impl/semaphore_capacity_control.hpp>
#include <userver/concurrent/queue_helpers.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

namespace impl {

template <typename T>
struct MpscQueueNode final : public SinglyLinkedBaseHook {
    explicit MpscQueueNode(T&& value) : value(std::move(value)) {}

    T value;
};

}  // namespace impl

/// @ingroup userver_concurrency
///
/// Multiple producer, single consumer queue
///
/// ## Example usage:
///
/// @snippet concurrent/mpsc_queue_test.cpp  Sample concurrent::MpscQueue usage
///
/// @see @ref scripts/docs/en/userver/synchronization.md
template <typename T>
class MpscQueue final : public std::enable_shared_from_this<MpscQueue<T>> {
    struct EmplaceEnabler final {
        // Disable {}-initialization in Queue's constructor
        explicit EmplaceEnabler() = default;
    };

    using Node = impl::MpscQueueNode<T>;

    using ProducerToken = impl::NoToken;
    using ConsumerToken = impl::NoToken;

    friend class Producer<MpscQueue, ProducerToken, EmplaceEnabler>;
    friend class Consumer<MpscQueue, ConsumerToken, EmplaceEnabler>;

public:
    static constexpr std::size_t kUnbounded = std::numeric_limits<std::size_t>::max();

    using ValueType = T;

    using Producer = concurrent::Producer<MpscQueue, ProducerToken, EmplaceEnabler>;
    using Consumer = concurrent::Consumer<MpscQueue, ConsumerToken, EmplaceEnabler>;
    using MultiProducer = concurrent::Producer<MpscQueue, impl::NoToken, EmplaceEnabler>;

    /// @cond
    // For internal use only
    explicit MpscQueue(std::size_t max_size, EmplaceEnabler /*unused*/)
        : remaining_capacity_(max_size), remaining_capacity_control_(remaining_capacity_) {}

    MpscQueue(MpscQueue&&) = delete;
    MpscQueue(const MpscQueue&) = delete;
    MpscQueue& operator=(MpscQueue&&) = delete;
    MpscQueue& operator=(const MpscQueue&) = delete;
    ~MpscQueue();
    /// @endcond

    /// Create a new queue
    static std::shared_ptr<MpscQueue> Create(std::size_t max_size = kUnbounded) {
        return std::make_shared<MpscQueue>(max_size, EmplaceEnabler{});
    }

    /// Get a `Producer` which makes it possible to push items into the queue.
    /// Can be called multiple times. The resulting `Producer` is not thread-safe,
    /// so you have to use multiple Producers of the same queue to simultaneously
    /// write from multiple coroutines/threads.
    ///
    /// @note `Producer` may outlive the queue and the `Consumer`.
    Producer GetProducer();

    /// Get a `MultiProducer` which makes it possible to push items into the
    /// queue. Can be called multiple times. The resulting `MultiProducer` is
    /// thread-safe, so it can be used simultaneously from multiple
    /// coroutines/threads.
    ///
    /// @note `MultiProducer` may outlive the queue and the `Consumer`.
    MultiProducer GetMultiProducer();

    /// Get a `Consumer` which makes it possible to read items from the queue.
    /// Can be called only once. You may not use the `Consumer` simultaneously
    /// from multiple coroutines/threads.
    ///
    /// @note `Consumer` may outlive the queue and producers.
    Consumer GetConsumer();

    /// @brief Sets the limit on the queue size, pushes over this limit will block
    /// @note This is a soft limit and may be slightly overrun under load.
    void SetSoftMaxSize(size_t size);

    /// @brief Gets the limit on the queue size
    [[nodiscard]] size_t GetSoftMaxSize() const;

    /// @brief Gets the approximate size of queue
    [[nodiscard]] size_t GetSizeApproximate() const;

private:
    bool Push(ProducerToken&, T&&, engine::Deadline);
    bool PushNoblock(ProducerToken&, T&&);
    bool DoPush(ProducerToken&, T&&);

    bool Pop(ConsumerToken&, T&, engine::Deadline);
    bool PopNoblock(ConsumerToken&, T&);
    bool DoPop(ConsumerToken&, T&);

    void MarkConsumerIsDead();
    void MarkProducerIsDead();

    impl::IntrusiveMpscQueue<Node> queue_{};
    engine::SingleConsumerEvent nonempty_event_{};
    engine::CancellableSemaphore remaining_capacity_;
    impl::SemaphoreCapacityControl remaining_capacity_control_;
    std::atomic<bool> consumer_is_created_{false};
    std::atomic<bool> consumer_is_created_and_dead_{false};
    std::atomic<bool> producer_is_created_and_dead_{false};
    std::atomic<size_t> producers_count_{0};
    std::atomic<size_t> size_{0};
};

template <typename T>
MpscQueue<T>::~MpscQueue() {
    UASSERT(consumer_is_created_and_dead_ || !consumer_is_created_);
    UASSERT(!producers_count_);
    // Clear remaining items in queue.
    while (const auto node = std::unique_ptr<Node>{queue_.TryPopBlocking()}) {
        remaining_capacity_.unlock_shared();
    }
}

template <typename T>
typename MpscQueue<T>::Producer MpscQueue<T>::GetProducer() {
    producers_count_++;
    producer_is_created_and_dead_ = false;
    nonempty_event_.Send();
    return Producer(this->shared_from_this(), EmplaceEnabler{});
}

template <typename T>
typename MpscQueue<T>::MultiProducer MpscQueue<T>::GetMultiProducer() {
    // MultiProducer and Producer are actually the same for MpscQueue, which is an
    // implementation detail.
    return GetProducer();
}

template <typename T>
typename MpscQueue<T>::Consumer MpscQueue<T>::GetConsumer() {
    UINVARIANT(!consumer_is_created_, "MpscQueue::Consumer must only be obtained a single time");
    consumer_is_created_ = true;
    return Consumer(this->shared_from_this(), EmplaceEnabler{});
}

template <typename T>
void MpscQueue<T>::SetSoftMaxSize(size_t max_size) {
    remaining_capacity_control_.SetCapacity(max_size);
}

template <typename T>
size_t MpscQueue<T>::GetSoftMaxSize() const {
    return remaining_capacity_control_.GetCapacity();
}

template <typename T>
size_t MpscQueue<T>::GetSizeApproximate() const {
    return size_;
}

template <typename T>
bool MpscQueue<T>::Push(ProducerToken& token, T&& value, engine::Deadline deadline) {
    return remaining_capacity_.try_lock_shared_until(deadline) && DoPush(token, std::move(value));
}

template <typename T>
bool MpscQueue<T>::PushNoblock(ProducerToken& token, T&& value) {
    return remaining_capacity_.try_lock_shared() && DoPush(token, std::move(value));
}

template <typename T>
bool MpscQueue<T>::DoPush(ProducerToken& /*unused*/, T&& value) {
    if (consumer_is_created_and_dead_) {
        remaining_capacity_.unlock_shared();
        return false;
    }

    auto node = std::make_unique<Node>(std::move(value));
    queue_.Push(*node);
    (void)node.release();

    ++size_;
    nonempty_event_.Send();

    return true;
}

template <typename T>
bool MpscQueue<T>::Pop(ConsumerToken& token, T& value, engine::Deadline deadline) {
    while (!DoPop(token, value)) {
        if (producer_is_created_and_dead_ || !nonempty_event_.WaitForEventUntil(deadline)) {
            // Producer might have pushed something in queue between .pop()
            // and !producer_is_created_and_dead_ check. Check twice to avoid
            // TOCTOU.
            return DoPop(token, value);
        }
    }
    return true;
}

template <typename T>
bool MpscQueue<T>::PopNoblock(ConsumerToken& token, T& value) {
    return DoPop(token, value);
}

template <typename T>
bool MpscQueue<T>::DoPop(ConsumerToken& /*unused*/, T& value) {
    if (const auto node = std::unique_ptr<Node>{queue_.TryPopWeak()}) {
        value = std::move(node->value);

        --size_;
        remaining_capacity_.unlock_shared();
        nonempty_event_.Reset();
        return true;
    }
    return false;
}

template <typename T>
void MpscQueue<T>::MarkConsumerIsDead() {
    consumer_is_created_and_dead_ = true;
    remaining_capacity_control_.SetCapacityOverride(0);
}

template <typename T>
void MpscQueue<T>::MarkProducerIsDead() {
    producer_is_created_and_dead_ = (--producers_count_ == 0);
    nonempty_event_.Send();
}

}  // namespace concurrent

USERVER_NAMESPACE_END
