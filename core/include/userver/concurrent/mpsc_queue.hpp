#pragma once

#include <atomic>
#include <memory>

#include <boost/lockfree/queue.hpp>

#include <userver/concurrent/impl/queue_helpers.hpp>
#include <userver/concurrent/impl/semaphore_capacity_control.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

namespace impl {

/// Helper template. Default implementation is straightforward.
template <typename T>
struct QueueHelper {
  using LockFreeQueue = boost::lockfree::queue<T>;

  static void Push(LockFreeQueue& queue, T&& value) {
    [[maybe_unused]] bool push_result = queue.push(std::move(value));
    UASSERT(push_result);
  }

  [[nodiscard]] static bool Pop(LockFreeQueue& queue, T& value) {
    return queue.pop(value);
  }

  static_assert(std::is_trivially_destructible_v<T>,
                "T has non-trivial destructor. Use "
                "MpscQueue<std::unique_ptr<T>> instead of MpscQueue<T>");
};

/// This partial specialization is helper's raison d'être. It allows
/// one to pass std::unique_ptr via Queue
template <typename T>
struct QueueHelper<std::unique_ptr<T>> {
  using LockFreeQueue = boost::lockfree::queue<T*>;

  static void Push(LockFreeQueue& queue, std::unique_ptr<T>&& value) {
    QueueHelper<T*>::Push(queue, value.release());
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
  }

  [[nodiscard]] static bool Pop(LockFreeQueue& queue,
                                std::unique_ptr<T>& value) {
    T* ptr{nullptr};
    if (!QueueHelper<T*>::Pop(queue, ptr)) return false;
    value.reset(ptr);
    return true;
  }
};

}  // namespace impl

/// @ingroup userver_concurrency
///
/// Multiple producer, single consumer queue
///
/// ## Example usage:
///
/// @snippet engine/mpsc_queue_test.cpp  Sample engine::MpscQueue usage
///
/// @see @ref md_en_userver_synchronization
template <typename T>
class MpscQueue final : public std::enable_shared_from_this<MpscQueue<T>> {
 private:
  using QueueHelper = impl::QueueHelper<T>;

  using ProducerToken = impl::NoToken;
  using ConsumerToken = impl::NoToken;

  class EmplaceEnabler {};

 public:
  static constexpr std::size_t kUnbounded =
      std::numeric_limits<std::size_t>::max();

  using ValueType = T;
  using Producer = impl::Producer<MpscQueue>;
  using Consumer = impl::Consumer<MpscQueue>;

  friend class impl::Producer<MpscQueue>;
  friend class impl::Consumer<MpscQueue>;

  explicit MpscQueue(std::size_t max_size, EmplaceEnabler /*unused*/)
      : remaining_capacity_(max_size),
        remaining_capacity_control_(remaining_capacity_) {}

  /// Create a new queue
  static std::shared_ptr<MpscQueue> Create(std::size_t max_size = kUnbounded) {
    return std::make_shared<MpscQueue>(max_size, EmplaceEnabler{});
  }

  ~MpscQueue();
  MpscQueue(const MpscQueue&) = delete;
  MpscQueue(MpscQueue&&) = delete;

  /// Can be called only once.
  ///
  /// Producer may outlive the queue and the consumer.
  Producer GetProducer();

  /// Can be called only once.
  ///
  /// Consumer may outlive the queue and the producer.
  Consumer GetConsumer();

  /// @brief Sets the limit on the queue size, pushes over this limit will block
  /// @note This is a soft limit and may be slightly overrun under load.
  void SetSoftMaxSize(size_t size);

  /// @brief Gets the limit on the queue size
  [[nodiscard]] size_t GetSoftMaxSize() const;

  [[nodiscard]] size_t GetSizeApproximate() const;

  [[deprecated("Use SetSoftMaxSize instead")]] void SetMaxLength(size_t length);

  [[deprecated("Use GetSoftMaxSize instead")]] size_t GetMaxLength() const;

  [[deprecated("Use GetSizeApproximate instead")]] size_t Size() const;

 private:
  bool Push(ProducerToken&, T&&, engine::Deadline);
  bool PushNoblock(ProducerToken&, T&&);
  bool DoPush(ProducerToken&, T&&);

  bool Pop(ConsumerToken&, T&, engine::Deadline);
  bool PopNoblock(ConsumerToken&, T&);
  bool DoPop(ConsumerToken&, T&);

  void MarkConsumerIsDead();
  void MarkProducerIsDead();

 private:
  // Resolves to boost::lockfree::queue<T> except for std::unique_ptr<T>
  // specialization. In that case, resolves to boost::lockfree::queue<T*>
  typename QueueHelper::LockFreeQueue queue_{1};
  engine::SingleConsumerEvent nonempty_event_;
  engine::Semaphore remaining_capacity_;
  concurrent::impl::SemaphoreCapacityControl remaining_capacity_control_;
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
  // Clear remaining items in queue. This will work for unique_ptr as well.
  T value;
  ConsumerToken temp_token{queue_};
  while (PopNoblock(temp_token, value)) {
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
typename MpscQueue<T>::Consumer MpscQueue<T>::GetConsumer() {
  UINVARIANT(!consumer_is_created_,
             "MpscQueue::Consumer must only be obtained a single time");
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
void MpscQueue<T>::SetMaxLength(size_t length) {
  SetSoftMaxSize(length);
}

template <typename T>
size_t MpscQueue<T>::GetMaxLength() const {
  return GetSoftMaxSize();
}

template <typename T>
size_t MpscQueue<T>::Size() const {
  return GetSizeApproximate();
}

template <typename T>
bool MpscQueue<T>::Push(ProducerToken& token, T&& value,
                        engine::Deadline deadline) {
  return !engine::current_task::ShouldCancel() &&
         remaining_capacity_.try_lock_shared_until(deadline) &&
         DoPush(token, std::move(value));
}

template <typename T>
bool MpscQueue<T>::PushNoblock(ProducerToken& token, T&& value) {
  return remaining_capacity_.try_lock_shared() &&
         DoPush(token, std::move(value));
}

template <typename T>
bool MpscQueue<T>::DoPush(ProducerToken& /*unused*/, T&& value) {
  if (consumer_is_created_and_dead_) {
    remaining_capacity_.unlock_shared();
    return false;
  }

  QueueHelper::Push(queue_, std::move(value));
  ++size_;
  nonempty_event_.Send();

  return true;
}

template <typename T>
bool MpscQueue<T>::Pop(ConsumerToken& token, T& value,
                       engine::Deadline deadline) {
  while (!DoPop(token, value)) {
    if (producer_is_created_and_dead_ ||
        !nonempty_event_.WaitForEventUntil(deadline)) {
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
  if (QueueHelper::Pop(queue_, value)) {
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
