#pragma once

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

namespace impl {

class MultiProducerToken final {
 public:
  template <typename T, typename Traits>
  explicit MultiProducerToken(moodycamel::ConcurrentQueue<T, Traits>&) {}
};

template <bool MultipleProducer, bool MultipleConsumer>
struct SimpleQueuePolicy {
  template <typename T>
  static constexpr std::size_t GetElementSize(const T&) {
    return 1;
  }

  static constexpr bool kIsMultipleProducer{MultipleProducer};
  static constexpr bool kIsMultipleConsumer{MultipleConsumer};
};

template <bool MultipleProducer, bool MultipleConsumer>
struct ContainerQueuePolicy {
  template <typename T>
  static std::size_t GetElementSize(const T& value) {
    return std::size(value);
  }

  static constexpr bool kIsMultipleProducer{MultipleProducer};
  static constexpr bool kIsMultipleConsumer{MultipleConsumer};
};

}  // namespace impl

/// Queue with single and multi producer/consumer options
///
/// @see @ref md_en_userver_synchronization
template <typename T, typename QueuePolicy>
class GenericQueue final
    : public std::enable_shared_from_this<GenericQueue<T, QueuePolicy>> {
  struct EmplaceEnabler final {
    // Disable {}-initialization in Queue's constructor
    explicit EmplaceEnabler() = default;
  };

  using ProducerToken =
      std::conditional_t<QueuePolicy::kIsMultipleProducer,
                         moodycamel::ProducerToken, impl::NoToken>;
  using ConsumerToken =
      std::conditional_t<QueuePolicy::kIsMultipleProducer,
                         moodycamel::ConsumerToken, impl::NoToken>;
  using MultiProducerToken = impl::MultiProducerToken;

  using SingleProducerToken =
      std::conditional_t<!QueuePolicy::kIsMultipleProducer,
                         moodycamel::ProducerToken, impl::NoToken>;

  friend class Producer<GenericQueue, ProducerToken, EmplaceEnabler>;
  friend class Producer<GenericQueue, MultiProducerToken, EmplaceEnabler>;
  friend class Consumer<GenericQueue, EmplaceEnabler>;

 public:
  using ValueType = T;

  using Producer =
      concurrent::Producer<GenericQueue, ProducerToken, EmplaceEnabler>;
  using Consumer = concurrent::Consumer<GenericQueue, EmplaceEnabler>;
  using MultiProducer =
      concurrent::Producer<GenericQueue, MultiProducerToken, EmplaceEnabler>;

  static constexpr std::size_t kUnbounded =
      std::numeric_limits<std::size_t>::max() / 4;

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
  static std::shared_ptr<GenericQueue> Create(
      std::size_t max_size = kUnbounded) {
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
    static_assert(QueuePolicy::kIsMultipleProducer,
                  "Trying to obtain MultiProducer for a single-producer queue");
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
    std::size_t old_consumers_count{};
    utils::AtomicUpdate(consumers_count_, [&](auto old_value) {
      old_consumers_count = old_value;
      return old_value == kCreatedAndDead ? 1 : old_value + 1;
    });

    if (old_consumers_count == kCreatedAndDead) {
      producer_side_.ResumeBlockingOnPush();
    }
    UASSERT(QueuePolicy::kIsMultipleConsumer || old_consumers_count != 1);
    return Consumer(this->shared_from_this(), EmplaceEnabler{});
  }

  /// @brief Sets the limit on the queue size, pushes over this limit will block
  /// @note This is a soft limit and may be slightly overrun under load.
  void SetSoftMaxSize(std::size_t max_size) {
    producer_side_.SetSoftMaxSize(std::min(max_size, kUnbounded));
  }

  /// @brief Gets the limit on the queue size
  std::size_t GetSoftMaxSize() const { return producer_side_.GetSoftMaxSize(); }

  /// @brief Gets the approximate size of queue
  std::size_t GetSizeApproximate() const {
    return producer_side_.GetSizeApproximate();
  }

 private:
  class SingleProducerSide;
  class MultiProducerSide;
  class SingleConsumerSide;
  class MultiConsumerSide;

  /// Proxy-class makes synchronization of Push operations in multi or single
  /// producer cases
  using ProducerSide =
      std::conditional_t<QueuePolicy::kIsMultipleProducer, MultiProducerSide,
                         SingleProducerSide>;

  /// Proxy-class makes synchronization of Pop operations in multi or single
  /// consumer cases
  using ConsumerSide =
      std::conditional_t<QueuePolicy::kIsMultipleConsumer, MultiConsumerSide,
                         SingleConsumerSide>;

  template <typename Token>
  [[nodiscard]] bool Push(Token& token, T&& value, engine::Deadline deadline) {
    return producer_side_.Push(token, std::move(value), deadline);
  }

  template <typename Token>
  [[nodiscard]] bool PushNoblock(Token& token, T&& value) {
    return producer_side_.PushNoblock(token, std::move(value));
  }

  [[nodiscard]] bool Pop(ConsumerToken& token, T& value,
                         engine::Deadline deadline) {
    return consumer_side_.Pop(token, value, deadline);
  }

  [[nodiscard]] bool PopNoblock(ConsumerToken& token, T& value) {
    return consumer_side_.PopNoblock(token, value);
  }

  void PrepareProducer() {
    std::size_t old_producers_count{};
    utils::AtomicUpdate(producers_count_, [&](auto old_value) {
      old_producers_count = old_value;
      return old_value == kCreatedAndDead ? 1 : old_value + 1;
    });

    if (old_producers_count == kCreatedAndDead) {
      consumer_side_.ResumeBlockingOnPop();
    }
    UASSERT(QueuePolicy::kIsMultipleProducer || old_producers_count != 1);
  }

  void MarkConsumerIsDead() {
    const auto new_consumers_count =
        utils::AtomicUpdate(consumers_count_, [](auto old_value) {
          return old_value == 1 ? kCreatedAndDead : old_value - 1;
        });
    if (new_consumers_count == kCreatedAndDead) {
      producer_side_.StopBlockingOnPush();
    }
  }

  void MarkProducerIsDead() {
    const auto new_producers_count =
        utils::AtomicUpdate(producers_count_, [](auto old_value) {
          return old_value == 1 ? kCreatedAndDead : old_value - 1;
        });
    if (new_producers_count == kCreatedAndDead) {
      consumer_side_.StopBlockingOnPop();
    }
  }

 public:  // TODO
  bool NoMoreConsumers() const { return consumers_count_ == kCreatedAndDead; }

  bool NoMoreProducers() const { return producers_count_ == kCreatedAndDead; }

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

  [[nodiscard]] bool DoPop(ConsumerToken& token, T& value) {
    bool success = false;
    if constexpr (QueuePolicy::kIsMultipleProducer) {
      success = queue_.try_dequeue(token, value);
    } else {
      // Substitute with our single producer token
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

  static constexpr std::size_t kCreatedAndDead =
      std::numeric_limits<std::size_t>::max();
  static constexpr std::size_t kSemaphoreUnlockValue =
      std::numeric_limits<std::size_t>::max() / 2;
};

// Single-producer ProducerSide implementation
template <typename T, typename QueuePolicy>
class GenericQueue<T, QueuePolicy>::SingleProducerSide final {
 public:
  explicit SingleProducerSide(GenericQueue& queue, std::size_t capacity)
      : queue_(queue), used_capacity_(0), total_capacity_(capacity) {}

  // Blocks if there is a consumer to Pop the current value and task
  // shouldn't cancel and queue if full
  template <typename Token>
  [[nodiscard]] bool Push(Token& token, T&& value, engine::Deadline deadline) {
    if (DoPush(token, std::move(value))) {
      return true;
    }

    return non_full_event_.WaitForEventUntil(deadline) &&
           // NOLINTNEXTLINE(bugprone-use-after-move)
           DoPush(token, std::move(value));
  }

  template <typename Token>
  [[nodiscard]] bool PushNoblock(Token& token, T&& value) {
    return DoPush(token, std::move(value));
  }

  void OnElementPopped(std::size_t released_capacity) {
    used_capacity_.fetch_sub(released_capacity);
    non_full_event_.Send();
  }

  void StopBlockingOnPush() {
    total_capacity_ += kSemaphoreUnlockValue;
    non_full_event_.Send();
  }

  void ResumeBlockingOnPush() { total_capacity_ -= kSemaphoreUnlockValue; }

  void SetSoftMaxSize(std::size_t new_capacity) {
    const auto old_capacity = total_capacity_.exchange(new_capacity);
    if (new_capacity > old_capacity) non_full_event_.Send();
  }

  std::size_t GetSoftMaxSize() const noexcept { return total_capacity_.load(); }

  std::size_t GetSizeApproximate() const noexcept {
    return used_capacity_.load();
  }

 private:
  template <typename Token>
  [[nodiscard]] bool DoPush(Token& token, T&& value) {
    const std::size_t value_size = QueuePolicy::GetElementSize(value);
    if (queue_.NoMoreConsumers() ||
        used_capacity_.load() + value_size > total_capacity_.load()) {
      return false;
    }

    used_capacity_.fetch_add(value_size);
    queue_.DoPush(token, std::move(value));
    non_full_event_.Reset();
    return true;
  }

  GenericQueue& queue_;
  engine::SingleConsumerEvent non_full_event_;
  std::atomic<std::size_t> used_capacity_;
  std::atomic<std::size_t> total_capacity_;
};

// Multi producer ProducerSide implementation
template <typename T, typename QueuePolicy>
class GenericQueue<T, QueuePolicy>::MultiProducerSide final {
 public:
  explicit MultiProducerSide(GenericQueue& queue, std::size_t capacity)
      : queue_(queue),
        remaining_capacity_(capacity),
        remaining_capacity_control_(remaining_capacity_) {}

  // Blocks if there is a consumer to Pop the current value and task
  // shouldn't cancel and queue if full
  template <typename Token>
  [[nodiscard]] bool Push(Token& token, T&& value, engine::Deadline deadline) {
    const std::size_t value_size = QueuePolicy::GetElementSize(value);
    return remaining_capacity_.try_lock_shared_until_count(deadline,
                                                           value_size) &&
           DoPush(token, std::move(value));
  }

  template <typename Token>
  [[nodiscard]] bool PushNoblock(Token& token, T&& value) {
    const std::size_t value_size = QueuePolicy::GetElementSize(value);
    return remaining_capacity_.try_lock_shared_count(value_size) &&
           DoPush(token, std::move(value));
  }

  void OnElementPopped(std::size_t value_size) {
    remaining_capacity_.unlock_shared_count(value_size);
  }

  void StopBlockingOnPush() {
    remaining_capacity_control_.SetCapacityOverride(0);
  }

  void ResumeBlockingOnPush() {
    remaining_capacity_control_.RemoveCapacityOverride();
  }

  void SetSoftMaxSize(std::size_t count) {
    remaining_capacity_control_.SetCapacity(count);
  }

  std::size_t GetSizeApproximate() const noexcept {
    return remaining_capacity_.UsedApprox();
  }

  std::size_t GetSoftMaxSize() const noexcept {
    return remaining_capacity_control_.GetCapacity();
  }

 private:
  template <typename Token>
  [[nodiscard]] bool DoPush(Token& token, T&& value) {
    const std::size_t value_size = QueuePolicy::GetElementSize(value);
    UASSERT(value_size > 0);
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

// Single consumer ConsumerSide implementation
template <typename T, typename QueuePolicy>
class GenericQueue<T, QueuePolicy>::SingleConsumerSide final {
 public:
  explicit SingleConsumerSide(GenericQueue& queue)
      : queue_(queue), element_count_(0) {}

  // Blocks only if queue is empty
  [[nodiscard]] bool Pop(ConsumerToken& token, T& value,
                         engine::Deadline deadline) {
    while (!DoPop(token, value)) {
      if (queue_.NoMoreProducers() ||
          !nonempty_event_.WaitForEventUntil(deadline)) {
        // Producer might have pushed something in queue between .pop()
        // and !producer_is_created_and_dead_ check. Check twice to avoid
        // TOCTOU.
        return DoPop(token, value);
      }
    }
    return true;
  }

  [[nodiscard]] bool PopNoblock(ConsumerToken& token, T& value) {
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
  [[nodiscard]] bool DoPop(ConsumerToken& token, T& value) {
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

// Multi consumer ConsumerSide implementation
template <typename T, typename QueuePolicy>
class GenericQueue<T, QueuePolicy>::MultiConsumerSide final {
 public:
  explicit MultiConsumerSide(GenericQueue& queue)
      : queue_(queue),
        element_count_(kUnbounded),
        element_count_control_(element_count_) {
    const bool success = element_count_.try_lock_shared_count(kUnbounded);
    UASSERT(success);
  }

  ~MultiConsumerSide() { element_count_.unlock_shared_count(kUnbounded); }

  // Blocks only if queue is empty
  [[nodiscard]] bool Pop(ConsumerToken& token, T& value,
                         engine::Deadline deadline) {
    return element_count_.try_lock_shared_until(deadline) &&
           DoPop(token, value);
  }

  [[nodiscard]] bool PopNoblock(ConsumerToken& token, T& value) {
    return element_count_.try_lock_shared() && DoPop(token, value);
  }

  void OnElementPushed() { element_count_.unlock_shared(); }

  void StopBlockingOnPop() {
    element_count_control_.SetCapacityOverride(kUnbounded +
                                               kSemaphoreUnlockValue);
  }

  void ResumeBlockingOnPop() {
    element_count_control_.RemoveCapacityOverride();
  }

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
  [[nodiscard]] bool DoPop(ConsumerToken& token, T& value) {
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
/// @see @ref md_en_userver_synchronization
template <typename T>
using NonFifoMpmcQueue = GenericQueue<T, impl::SimpleQueuePolicy<true, true>>;

/// @ingroup userver_concurrency
///
/// @brief Non FIFO multiple producers single consumer queue.
///
/// @see concurrent::NonFifoMpmcQueue for the description of what NonFifo means.
/// @see @ref md_en_userver_synchronization
template <typename T>
using NonFifoMpscQueue = GenericQueue<T, impl::SimpleQueuePolicy<true, false>>;

/// @ingroup userver_concurrency
///
/// @brief Single producer multiple consumers queue.
///
/// @see @ref md_en_userver_synchronization
template <typename T>
using SpmcQueue = GenericQueue<T, impl::SimpleQueuePolicy<false, true>>;

/// @ingroup userver_concurrency
///
/// @brief Single producer single consumer queue.
///
/// @see @ref md_en_userver_synchronization
template <typename T>
using SpscQueue = GenericQueue<T, impl::SimpleQueuePolicy<false, false>>;

/// @ingroup userver_concurrency
///
/// @brief Single producer single consumer queue of std::string which is bounded
/// bytes inside.
///
/// @see @ref md_en_userver_synchronization
using StringStreamQueue =
    GenericQueue<std::string, impl::ContainerQueuePolicy<false, false>>;

}  // namespace concurrent

USERVER_NAMESPACE_END
