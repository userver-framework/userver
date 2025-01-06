#pragma once

/// @file userver/concurrent/queue.hpp
/// @brief @copybrief concurrent::GenericQueue

#include <atomic>
#include <limits>
#include <memory>

#include <moodycamel/concurrentqueue.h>

#include <userver/concurrent/impl/semaphore_capacity_control.hpp>
#include <userver/concurrent/queue_helpers.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/atomic.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

/// @brief Represents the manner in which the queue enforces the configured
/// max size limit.
/// @see @ref GenericQueue
/// @see @ref DefaultQueuePolicy
enum class QueueMaxSizeMode {
    /// No support for setting max size. Fastest.
    kNone,

    /// Supports dynamically changing max size; supports awaiting non-fullness
    /// in producers. Slightly slower than @ref kNone.
    kDynamicSync,
};

/// @brief The default queue policy for @ref GenericQueue.
/// Custom queue policies must inherit from it.
struct DefaultQueuePolicy {
    /// If `true`, the queue gains support for multiple concurrent producers,
    /// which adds some synchronization overhead. This also makes the queue
    /// non-FIFO, because consumer(s) will randomly prioritize some producers
    /// over the other ones.
    static constexpr bool kIsMultipleProducer{false};

    /// If `true`, the queue gains support for multiple concurrent consumers,
    /// which adds some synchronization overhead.
    static constexpr bool kIsMultipleConsumer{false};

    /// Whether the queue has support for max size,
    /// which adds some synchronization overhead.
    static constexpr QueueMaxSizeMode kMaxSizeMode = QueueMaxSizeMode::kNone;

    /// Should return the size of an element, which accounts towards
    /// the capacity limit. Only makes sense to set if `kHasMaxSize == true`.
    ///
    /// For the vast majority of queues, capacity is naturally counted
    /// in queue elements, e.g. `GetSoftMaxSize() == 1000` means that we want
    /// no more than 1000 elements in the queue.
    ///
    /// Sometimes we want a more complex limit, e.g. for a queue of strings we
    /// want the total number of bytes in all the strings to be limited.
    /// In that case we can set the element size equal to its `std::size`.
    ///
    /// @note Returning anything other than 1 adds some synchronization overhead.
    /// @warning An element changing its capacity while inside the queue is UB.
    /// @warning Overflow in the total queue size is UB.
    template <typename T>
    static constexpr std::size_t GetElementSize(const T&) {
        return 1;
    }
};

namespace impl {

template <bool MultipleProducer, bool MultipleConsumer>
struct SimpleQueuePolicy : public DefaultQueuePolicy {
    static constexpr bool kIsMultipleProducer{MultipleProducer};
    static constexpr bool kIsMultipleConsumer{MultipleConsumer};
    static constexpr auto kMaxSizeMode = QueueMaxSizeMode::kDynamicSync;
};

template <bool MultipleProducer, bool MultipleConsumer>
struct NoMaxSizeQueuePolicy : public DefaultQueuePolicy {
    static constexpr bool kIsMultipleProducer{MultipleProducer};
    static constexpr bool kIsMultipleConsumer{MultipleConsumer};
    static constexpr auto kMaxSizeMode = QueueMaxSizeMode::kNone;
};

template <bool MultipleProducer, bool MultipleConsumer>
struct ContainerQueuePolicy : public DefaultQueuePolicy {
    template <typename T>
    static std::size_t GetElementSize(const T& value) {
        return std::size(value);
    }

    static constexpr bool kIsMultipleProducer{MultipleProducer};
    static constexpr bool kIsMultipleConsumer{MultipleConsumer};
};

}  // namespace impl

/// @brief Queue with single and multi producer/consumer options.
///
/// @tparam T element type
/// @tparam QueuePolicy policy type, see @ref DefaultQueuePolicy for details
///
/// On practice, instead of using `GenericQueue` directly, use an alias:
///
/// * concurrent::NonFifoMpmcQueue
/// * concurrent::NonFifoMpscQueue
/// * concurrent::SpmcQueue
/// * concurrent::SpscQueue
/// * concurrent::UnboundedNonFifoMpscQueue
/// * concurrent::UnboundedSpmcQueue
/// * concurrent::UnboundedSpscQueue
/// * concurrent::StringStreamQueue
///
/// @see @ref concurrent_queues
template <typename T, typename QueuePolicy>
class GenericQueue final : public std::enable_shared_from_this<GenericQueue<T, QueuePolicy>> {
    struct EmplaceEnabler final {
        // Disable {}-initialization in Queue's constructor
        explicit EmplaceEnabler() = default;
    };

    static_assert(
        std::is_base_of_v<DefaultQueuePolicy, QueuePolicy>,
        "QueuePolicy must inherit from concurrent::DefaultQueuePolicy"
    );

    static constexpr QueueMaxSizeMode kMaxSizeMode = QueuePolicy::kMaxSizeMode;

    using ProducerToken =
        std::conditional_t<QueuePolicy::kIsMultipleProducer, moodycamel::ProducerToken, impl::NoToken>;
    using ConsumerToken =
        std::conditional_t<QueuePolicy::kIsMultipleProducer, moodycamel::ConsumerToken, impl::NoToken>;
    using MultiProducerToken = impl::MultiToken;
    using MultiConsumerToken = std::conditional_t<QueuePolicy::kIsMultipleProducer, impl::MultiToken, impl::NoToken>;

    using SingleProducerToken =
        std::conditional_t<!QueuePolicy::kIsMultipleProducer, moodycamel::ProducerToken, impl::NoToken>;

    friend class Producer<GenericQueue, ProducerToken, EmplaceEnabler>;
    friend class Producer<GenericQueue, MultiProducerToken, EmplaceEnabler>;
    friend class Consumer<GenericQueue, ConsumerToken, EmplaceEnabler>;
    friend class Consumer<GenericQueue, MultiConsumerToken, EmplaceEnabler>;

public:
    using ValueType = T;

    using Producer = concurrent::Producer<GenericQueue, ProducerToken, EmplaceEnabler>;
    using Consumer = concurrent::Consumer<GenericQueue, ConsumerToken, EmplaceEnabler>;
    using MultiProducer = concurrent::Producer<GenericQueue, MultiProducerToken, EmplaceEnabler>;
    using MultiConsumer = concurrent::Consumer<GenericQueue, MultiConsumerToken, EmplaceEnabler>;

    static constexpr std::size_t kUnbounded = std::numeric_limits<std::size_t>::max() / 4;

    /// @cond
    // For internal use only
    explicit GenericQueue(std::size_t max_size, EmplaceEnabler /*unused*/)
        : queue_(),
          single_producer_token_(queue_),
          producer_side_(*this, std::min(max_size, kUnbounded)),
          consumer_side_(*this) {}

    ~GenericQueue() {
        UASSERT(consumers_count_ == kCreatedAndDead || !consumers_count_);
        UASSERT(producers_count_ == kCreatedAndDead || !producers_count_);

        if (producers_count_ == kCreatedAndDead) {
            // To allow reading the remaining items
            consumer_side_.ResumeBlockingOnPop();
        }

        // Clear remaining items in queue
        T value;
        ConsumerToken token{queue_};
        while (consumer_side_.PopNoblock(token, value)) {
        }
    }

    GenericQueue(GenericQueue&&) = delete;
    GenericQueue(const GenericQueue&) = delete;
    GenericQueue& operator=(GenericQueue&&) = delete;
    GenericQueue& operator=(const GenericQueue&) = delete;
    /// @endcond

    /// Create a new queue
    static std::shared_ptr<GenericQueue> Create(std::size_t max_size = kUnbounded) {
        return std::make_shared<GenericQueue>(max_size, EmplaceEnabler{});
    }

    /// Get a `Producer` which makes it possible to push items into the queue.
    /// Can be called multiple times. The resulting `Producer` is not thread-safe,
    /// so you have to use multiple Producers of the same queue to simultaneously
    /// write from multiple coroutines/threads.
    ///
    /// @note `Producer` may outlive the queue and consumers.
    Producer GetProducer() {
        PrepareProducer();
        return Producer(this->shared_from_this(), EmplaceEnabler{});
    }

    /// Get a `MultiProducer` which makes it possible to push items into the
    /// queue. Can be called multiple times. The resulting `MultiProducer` is
    /// thread-safe, so it can be used simultaneously from multiple
    /// coroutines/threads.
    ///
    /// @note `MultiProducer` may outlive the queue and consumers.
    ///
    /// @note Prefer `Producer` tokens when possible, because `MultiProducer`
    /// token incurs some overhead.
    MultiProducer GetMultiProducer() {
        static_assert(QueuePolicy::kIsMultipleProducer, "Trying to obtain MultiProducer for a single-producer queue");
        PrepareProducer();
        return MultiProducer(this->shared_from_this(), EmplaceEnabler{});
    }

    /// Get a `Consumer` which makes it possible to read items from the queue.
    /// Can be called multiple times. The resulting `Consumer` is not thread-safe,
    /// so you have to use multiple `Consumer`s of the same queue to
    /// simultaneously write from multiple coroutines/threads.
    ///
    /// @note `Consumer` may outlive the queue and producers.
    Consumer GetConsumer() {
        PrepareConsumer();
        return Consumer(this->shared_from_this(), EmplaceEnabler{});
    }

    /// Get a `MultiConsumer` which makes it possible to read items from the
    /// queue. Can be called multiple times. The resulting `MultiConsumer` is
    /// thread-safe, so it can be used simultaneously from multiple
    /// coroutines/threads.
    ///
    /// @note `MultiConsumer` may outlive the queue and producers.
    ///
    /// @note Prefer `Consumer` tokens when possible, because `MultiConsumer`
    /// token incurs some overhead.
    MultiConsumer GetMultiConsumer() {
        static_assert(QueuePolicy::kIsMultipleConsumer, "Trying to obtain MultiConsumer for a single-consumer queue");
        PrepareConsumer();
        return MultiConsumer(this->shared_from_this(), EmplaceEnabler{});
    }

    /// @brief Sets the limit on the queue size, pushes over this limit will block
    /// @note This is a soft limit and may be slightly overrun under load.
    void SetSoftMaxSize(std::size_t max_size) { producer_side_.SetSoftMaxSize(std::min(max_size, kUnbounded)); }

    /// @brief Gets the limit on the queue size
    std::size_t GetSoftMaxSize() const { return producer_side_.GetSoftMaxSize(); }

    /// @brief Gets the approximate size of queue
    std::size_t GetSizeApproximate() const { return producer_side_.GetSizeApproximate(); }

private:
    class SingleProducerSide;
    class MultiProducerSide;
    class NoMaxSizeProducerSide;
    class SingleConsumerSide;
    class MultiConsumerSide;

    /// Proxy-class makes synchronization of Push operations in multi or single
    /// producer cases
    using ProducerSide = std::conditional_t<
        kMaxSizeMode == QueueMaxSizeMode::kNone,
        NoMaxSizeProducerSide,
        std::conditional_t<  //
            QueuePolicy::kIsMultipleProducer,
            MultiProducerSide,
            SingleProducerSide>>;

    /// Proxy-class makes synchronization of Pop operations in multi or single
    /// consumer cases
    using ConsumerSide = std::conditional_t<QueuePolicy::kIsMultipleConsumer, MultiConsumerSide, SingleConsumerSide>;

    template <typename Token>
    [[nodiscard]] bool Push(Token& token, T&& value, engine::Deadline deadline) {
        const std::size_t value_size = QueuePolicy::GetElementSize(value);
        UASSERT(value_size > 0);
        return producer_side_.Push(token, std::move(value), deadline, value_size);
    }

    template <typename Token>
    [[nodiscard]] bool PushNoblock(Token& token, T&& value) {
        const std::size_t value_size = QueuePolicy::GetElementSize(value);
        UASSERT(value_size > 0);
        return producer_side_.PushNoblock(token, std::move(value), value_size);
    }

    template <typename Token>
    [[nodiscard]] bool Pop(Token& token, T& value, engine::Deadline deadline) {
        return consumer_side_.Pop(token, value, deadline);
    }

    template <typename Token>
    [[nodiscard]] bool PopNoblock(Token& token, T& value) {
        return consumer_side_.PopNoblock(token, value);
    }

    void PrepareProducer() {
        std::size_t old_producers_count{};
        utils::AtomicUpdate(producers_count_, [&](auto old_value) {
            UINVARIANT(QueuePolicy::kIsMultipleProducer || old_value != 1, "Incorrect usage of queue producers");
            old_producers_count = old_value;
            return old_value == kCreatedAndDead ? 1 : old_value + 1;
        });

        if (old_producers_count == kCreatedAndDead) {
            consumer_side_.ResumeBlockingOnPop();
        }
    }

    void PrepareConsumer() {
        std::size_t old_consumers_count{};
        utils::AtomicUpdate(consumers_count_, [&](auto old_value) {
            UINVARIANT(QueuePolicy::kIsMultipleConsumer || old_value != 1, "Incorrect usage of queue consumers");
            old_consumers_count = old_value;
            return old_value == kCreatedAndDead ? 1 : old_value + 1;
        });

        if (old_consumers_count == kCreatedAndDead) {
            producer_side_.ResumeBlockingOnPush();
        }
    }

    void MarkConsumerIsDead() {
        const auto new_consumers_count = utils::AtomicUpdate(consumers_count_, [](auto old_value) {
            return old_value == 1 ? kCreatedAndDead : old_value - 1;
        });
        if (new_consumers_count == kCreatedAndDead) {
            producer_side_.StopBlockingOnPush();
        }
    }

    void MarkProducerIsDead() {
        const auto new_producers_count = utils::AtomicUpdate(producers_count_, [](auto old_value) {
            return old_value == 1 ? kCreatedAndDead : old_value - 1;
        });
        if (new_producers_count == kCreatedAndDead) {
            consumer_side_.StopBlockingOnPop();
        }
    }

public:  // TODO
    /// @cond
    bool NoMoreConsumers() const { return consumers_count_ == kCreatedAndDead; }

    bool NoMoreProducers() const { return producers_count_ == kCreatedAndDead; }
    /// @endcond

private:
    template <typename Token>
    void DoPush(Token& token, T&& value) {
        if constexpr (std::is_same_v<Token, moodycamel::ProducerToken>) {
            static_assert(QueuePolicy::kIsMultipleProducer);
            queue_.enqueue(token, std::move(value));
        } else if constexpr (std::is_same_v<Token, MultiProducerToken>) {
            static_assert(QueuePolicy::kIsMultipleProducer);
            queue_.enqueue(std::move(value));
        } else {
            static_assert(std::is_same_v<Token, impl::NoToken>);
            static_assert(!QueuePolicy::kIsMultipleProducer);
            queue_.enqueue(single_producer_token_, std::move(value));
        }

        consumer_side_.OnElementPushed();
    }

    template <typename Token>
    [[nodiscard]] bool DoPop(Token& token, T& value) {
        bool success{};

        if constexpr (std::is_same_v<Token, moodycamel::ConsumerToken>) {
            static_assert(QueuePolicy::kIsMultipleProducer);
            success = queue_.try_dequeue(token, value);
        } else if constexpr (std::is_same_v<Token, impl::MultiToken>) {
            static_assert(QueuePolicy::kIsMultipleProducer);
            success = queue_.try_dequeue(value);
        } else {
            static_assert(std::is_same_v<Token, impl::NoToken>);
            static_assert(!QueuePolicy::kIsMultipleProducer);
            success = queue_.try_dequeue_from_producer(single_producer_token_, value);
        }

        if (success) {
            producer_side_.OnElementPopped(QueuePolicy::GetElementSize(value));
            return true;
        }

        return false;
    }

    moodycamel::ConcurrentQueue<T> queue_{1};
    std::atomic<std::size_t> consumers_count_{0};
    std::atomic<std::size_t> producers_count_{0};

    SingleProducerToken single_producer_token_;

    ProducerSide producer_side_;
    ConsumerSide consumer_side_;

    static constexpr std::size_t kCreatedAndDead = std::numeric_limits<std::size_t>::max();
    static constexpr std::size_t kSemaphoreUnlockValue = std::numeric_limits<std::size_t>::max() / 2;
};

template <typename T, typename QueuePolicy>
class GenericQueue<T, QueuePolicy>::SingleProducerSide final {
public:
    SingleProducerSide(GenericQueue& queue, std::size_t capacity)
        : queue_(queue), used_capacity_(0), total_capacity_(capacity) {}

    // Blocks if there is a consumer to Pop the current value and task
    // shouldn't cancel and queue if full
    template <typename Token>
    [[nodiscard]] bool Push(Token& token, T&& value, engine::Deadline deadline, std::size_t value_size) {
        bool no_more_consumers = false;
        const bool success = non_full_event_.WaitUntil(deadline, [&] {
            if (queue_.NoMoreConsumers()) {
                no_more_consumers = true;
                return true;
            }
            if (DoPush(token, std::move(value), value_size)) {
                return true;
            }
            return false;
        });
        return success && !no_more_consumers;
    }

    template <typename Token>
    [[nodiscard]] bool PushNoblock(Token& token, T&& value, std::size_t value_size) {
        return !queue_.NoMoreConsumers() && DoPush(token, std::move(value), value_size);
    }

    void OnElementPopped(std::size_t released_capacity) {
        used_capacity_.fetch_sub(released_capacity);
        non_full_event_.Send();
    }

    void StopBlockingOnPush() { non_full_event_.Send(); }

    void ResumeBlockingOnPush() {}

    void SetSoftMaxSize(std::size_t new_capacity) {
        const auto old_capacity = total_capacity_.exchange(new_capacity);
        if (new_capacity > old_capacity) non_full_event_.Send();
    }

    std::size_t GetSoftMaxSize() const noexcept { return total_capacity_.load(); }

    std::size_t GetSizeApproximate() const noexcept { return used_capacity_.load(); }

private:
    template <typename Token>
    [[nodiscard]] bool DoPush(Token& token, T&& value, std::size_t value_size) {
        if (used_capacity_.load() + value_size > total_capacity_.load()) {
            return false;
        }

        used_capacity_.fetch_add(value_size);
        queue_.DoPush(token, std::move(value));
        return true;
    }

    GenericQueue& queue_;
    engine::SingleConsumerEvent non_full_event_;
    std::atomic<std::size_t> used_capacity_;
    std::atomic<std::size_t> total_capacity_;
};

template <typename T, typename QueuePolicy>
class GenericQueue<T, QueuePolicy>::MultiProducerSide final {
public:
    MultiProducerSide(GenericQueue& queue, std::size_t capacity)
        : queue_(queue), remaining_capacity_(capacity), remaining_capacity_control_(remaining_capacity_) {}

    // Blocks if there is a consumer to Pop the current value and task
    // shouldn't cancel and queue if full
    template <typename Token>
    [[nodiscard]] bool Push(Token& token, T&& value, engine::Deadline deadline, std::size_t value_size) {
        return remaining_capacity_.try_lock_shared_until_count(deadline, value_size) &&
               DoPush(token, std::move(value), value_size);
    }

    template <typename Token>
    [[nodiscard]] bool PushNoblock(Token& token, T&& value, std::size_t value_size) {
        return remaining_capacity_.try_lock_shared_count(value_size) && DoPush(token, std::move(value), value_size);
    }

    void OnElementPopped(std::size_t value_size) { remaining_capacity_.unlock_shared_count(value_size); }

    void StopBlockingOnPush() { remaining_capacity_control_.SetCapacityOverride(0); }

    void ResumeBlockingOnPush() { remaining_capacity_control_.RemoveCapacityOverride(); }

    void SetSoftMaxSize(std::size_t count) { remaining_capacity_control_.SetCapacity(count); }

    std::size_t GetSizeApproximate() const noexcept { return remaining_capacity_.UsedApprox(); }

    std::size_t GetSoftMaxSize() const noexcept { return remaining_capacity_control_.GetCapacity(); }

private:
    template <typename Token>
    [[nodiscard]] bool DoPush(Token& token, T&& value, std::size_t value_size) {
        if (queue_.NoMoreConsumers()) {
            remaining_capacity_.unlock_shared_count(value_size);
            return false;
        }

        queue_.DoPush(token, std::move(value));
        return true;
    }

    GenericQueue& queue_;
    engine::CancellableSemaphore remaining_capacity_;
    concurrent::impl::SemaphoreCapacityControl remaining_capacity_control_;
};

template <typename T, typename QueuePolicy>
class GenericQueue<T, QueuePolicy>::NoMaxSizeProducerSide final {
public:
    NoMaxSizeProducerSide(GenericQueue& queue, std::size_t max_size) : queue_(queue) { SetSoftMaxSize(max_size); }

    template <typename Token>
    [[nodiscard]] bool Push(Token& token, T&& value, engine::Deadline /*deadline*/, std::size_t /*value_size*/) {
        if (queue_.NoMoreConsumers()) {
            return false;
        }

        queue_.DoPush(token, std::move(value));
        return true;
    }

    template <typename Token>
    [[nodiscard]] bool PushNoblock(Token& token, T&& value, std::size_t value_size) {
        return Push(token, std::move(value), engine::Deadline{}, value_size);
    }

    void OnElementPopped(std::size_t /*released_capacity*/) {}

    void StopBlockingOnPush() {}

    void ResumeBlockingOnPush() {}

    void SetSoftMaxSize(std::size_t new_capacity) {
        UINVARIANT(new_capacity == kUnbounded, "Cannot set max size for a queue with QueueMaxSizeMode::kNone");
    }

    std::size_t GetSoftMaxSize() const noexcept { return kUnbounded; }

    std::size_t GetSizeApproximate() const noexcept { return queue_.queue_.size_approx(); }

private:
    GenericQueue& queue_;
};

template <typename T, typename QueuePolicy>
class GenericQueue<T, QueuePolicy>::SingleConsumerSide final {
public:
    explicit SingleConsumerSide(GenericQueue& queue) : queue_(queue), element_count_(0) {}

    // Blocks only if queue is empty
    template <typename Token>
    [[nodiscard]] bool Pop(Token& token, T& value, engine::Deadline deadline) {
        bool no_more_producers = false;
        const bool success = nonempty_event_.WaitUntil(deadline, [&] {
            if (DoPop(token, value)) {
                return true;
            }
            if (queue_.NoMoreProducers()) {
                // Producer might have pushed something in queue between .pop()
                // and !producer_is_created_and_dead_ check. Check twice to avoid
                // TOCTOU.
                if (!DoPop(token, value)) {
                    no_more_producers = true;
                }
                return true;
            }
            return false;
        });
        return success && !no_more_producers;
    }

    template <typename Token>
    [[nodiscard]] bool PopNoblock(Token& token, T& value) {
        return DoPop(token, value);
    }

    void OnElementPushed() {
        ++element_count_;
        nonempty_event_.Send();
    }

    void StopBlockingOnPop() { nonempty_event_.Send(); }

    void ResumeBlockingOnPop() {}

    std::size_t GetElementCount() const { return element_count_; }

private:
    template <typename Token>
    [[nodiscard]] bool DoPop(Token& token, T& value) {
        if (queue_.DoPop(token, value)) {
            --element_count_;
            nonempty_event_.Reset();
            return true;
        }
        return false;
    }

    GenericQueue& queue_;
    engine::SingleConsumerEvent nonempty_event_;
    std::atomic<std::size_t> element_count_;
};

template <typename T, typename QueuePolicy>
class GenericQueue<T, QueuePolicy>::MultiConsumerSide final {
public:
    explicit MultiConsumerSide(GenericQueue& queue)
        : queue_(queue), element_count_(kUnbounded), element_count_control_(element_count_) {
        const bool success = element_count_.try_lock_shared_count(kUnbounded);
        UASSERT(success);
    }

    ~MultiConsumerSide() { element_count_.unlock_shared_count(kUnbounded); }

    // Blocks only if queue is empty
    template <typename Token>
    [[nodiscard]] bool Pop(Token& token, T& value, engine::Deadline deadline) {
        return element_count_.try_lock_shared_until(deadline) && DoPop(token, value);
    }

    template <typename Token>
    [[nodiscard]] bool PopNoblock(Token& token, T& value) {
        return element_count_.try_lock_shared() && DoPop(token, value);
    }

    void OnElementPushed() { element_count_.unlock_shared(); }

    void StopBlockingOnPop() { element_count_control_.SetCapacityOverride(kUnbounded + kSemaphoreUnlockValue); }

    void ResumeBlockingOnPop() { element_count_control_.RemoveCapacityOverride(); }

    std::size_t GetElementCount() const {
        const std::size_t cur_element_count = element_count_.RemainingApprox();
        if (cur_element_count < kUnbounded) {
            return cur_element_count;
        } else if (cur_element_count <= kSemaphoreUnlockValue) {
            return 0;
        }
        return cur_element_count - kSemaphoreUnlockValue;
    }

private:
    template <typename Token>
    [[nodiscard]] bool DoPop(Token& token, T& value) {
        while (true) {
            if (queue_.DoPop(token, value)) {
                return true;
            }
            if (queue_.NoMoreProducers()) {
                element_count_.unlock_shared();
                return false;
            }
            // We can get here if another consumer steals our element, leaving another
            // element in a Moodycamel sub-queue that we have already passed.
        }
    }

    GenericQueue& queue_;
    engine::CancellableSemaphore element_count_;
    concurrent::impl::SemaphoreCapacityControl element_count_control_;
};

/// @ingroup userver_concurrency
///
/// @brief Non FIFO multiple producers multiple consumers queue.
///
/// Items from the same producer are always delivered in the production order.
/// Items from different producers (or when using a `MultiProducer` token) are
/// delivered in an unspecified order. In other words, FIFO order is maintained
/// only within producers, but not between them. This may lead to increased peak
/// latency of item processing.
///
/// In exchange for this, the queue has lower contention and increased
/// throughput compared to a conventional lock-free queue.
///
/// @see @ref scripts/docs/en/userver/synchronization.md
template <typename T>
using NonFifoMpmcQueue = GenericQueue<T, impl::SimpleQueuePolicy<true, true>>;

/// @ingroup userver_concurrency
///
/// @brief Non FIFO multiple producers single consumer queue.
///
/// @see concurrent::NonFifoMpmcQueue for the description of what NonFifo means.
/// @see @ref scripts/docs/en/userver/synchronization.md
template <typename T>
using NonFifoMpscQueue = GenericQueue<T, impl::SimpleQueuePolicy<true, false>>;

/// @ingroup userver_concurrency
///
/// @brief Single producer multiple consumers queue.
///
/// @see @ref scripts/docs/en/userver/synchronization.md
template <typename T>
using SpmcQueue = GenericQueue<T, impl::SimpleQueuePolicy<false, true>>;

/// @ingroup userver_concurrency
///
/// @brief Single producer single consumer queue.
///
/// @see @ref scripts/docs/en/userver/synchronization.md
template <typename T>
using SpscQueue = GenericQueue<T, impl::SimpleQueuePolicy<false, false>>;

namespace impl {

/// @ingroup userver_concurrency
///
/// @brief Like @see NonFifoMpmcQueue, but does not support setting max size and is thus slightly faster.
///
/// @warning The current implementation suffers from performance issues in multi-producer scenario under heavy load.
/// Precisely speaking, producers always take priority over consumers (breaking thread fairness), and consumers starve,
/// leading to increased latencies to the point of OOM. Use other queue types (unbounded or not) for the time being.
///
/// @see @ref scripts/docs/en/userver/synchronization.md
template <typename T>
using UnfairUnboundedNonFifoMpmcQueue = GenericQueue<T, impl::NoMaxSizeQueuePolicy<true, true>>;

}  // namespace impl

/// @ingroup userver_concurrency
///
/// @brief Like @see NonFifoMpscQueue, but does not support setting max size and is thus slightly faster.
///
/// @see @ref scripts/docs/en/userver/synchronization.md
template <typename T>
using UnboundedNonFifoMpscQueue = GenericQueue<T, impl::NoMaxSizeQueuePolicy<true, false>>;

/// @ingroup userver_concurrency
///
/// @brief Like @see SpmcQueue, but does not support setting max size and is thus slightly faster.
///
/// @see @ref scripts/docs/en/userver/synchronization.md
template <typename T>
using UnboundedSpmcQueue = GenericQueue<T, impl::NoMaxSizeQueuePolicy<false, true>>;

/// @ingroup userver_concurrency
///
/// @brief Like @see SpscQueue, but does not support setting max size and is thus slightly faster.
///
/// @see @ref scripts/docs/en/userver/synchronization.md
template <typename T>
using UnboundedSpscQueue = GenericQueue<T, impl::NoMaxSizeQueuePolicy<false, false>>;

/// @ingroup userver_concurrency
///
/// @brief Single producer single consumer queue of std::string which is bounded by the total bytes inside the strings.
///
/// @see @ref scripts/docs/en/userver/synchronization.md
using StringStreamQueue = GenericQueue<std::string, impl::ContainerQueuePolicy<false, false>>;

}  // namespace concurrent

USERVER_NAMESPACE_END
