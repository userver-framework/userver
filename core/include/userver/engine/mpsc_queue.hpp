#pragma once

#include <atomic>
#include <memory>
#include <optional>

#include <boost/lockfree/queue.hpp>

#include <userver/engine/condition_variable.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utils/assert.hpp>

namespace engine {

namespace impl {
/// Helper template. Default implemetation is straightforward.
template <typename T>
struct QueueHelper {
  using LockFreeQueue = boost::lockfree::queue<T>;
  static void Push(LockFreeQueue& queue, T&& value) {
    queue.push(std::move(value));
  }
  [[nodiscard]] static bool Pop(LockFreeQueue& queue, T& value) {
    return queue.pop(value);
  }

  static_assert(boost::has_trivial_destructor<T>::value,
                "T has non-trivial destructor. Use "
                "MpscQueue<std::unique_ptr<T>> instead of MpscQueue<T> or use "
                "another T.");
};

/// This partial specialization is helper's raison d'être. It allows
/// one to pass std::unique_ptr via MpscQueue
template <typename T>
struct QueueHelper<std::unique_ptr<T>> {
  using LockFreeQueue = boost::lockfree::queue<T*>;
  static void Push(LockFreeQueue& queue, std::unique_ptr<T>&& value) {
    QueueHelper<T*>::Push(queue, value.get());
    [[maybe_unused]] auto ptr = value.release();
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

/// Multple producer, single consumer queue
///
/// ## Example usage:
///
/// @snippet engine/mpsc_queue_test.cpp  Sample engine::MpscQueue usage
///
/// @see @ref md_en_userver_synchronization
template <typename T>
class MpscQueue final : public std::enable_shared_from_this<MpscQueue<T>> {
  using QueueHelper = impl::QueueHelper<T>;

 public:
  ~MpscQueue();

  MpscQueue(const MpscQueue&) = delete;

  MpscQueue(MpscQueue&&) = delete;

  /// Create a new queue
  static std::shared_ptr<MpscQueue> Create();

  class Producer final {
   public:
    Producer(std::shared_ptr<MpscQueue<T>> queue) : queue_(std::move(queue)) {}

    Producer(const Producer&) = delete;

    Producer(Producer&&) noexcept = default;

    ~Producer() {
      if (queue_) queue_->MarkProducerIsDead();
    }

    /// Push element into queue. May block if queue is full, but the consumer is
    /// alive.
    /// @returns whether the consumer was alive and push succeeded
    /// before the deadline.
    bool Push(T&& value, Deadline deadline = {}) {
      return queue_->Push(std::move(value), deadline, std::nullopt);
    }

    /// Push element into queue with max length override.
    /// May block if queue is full, but the consumer is alive.
    /// @param max_len used instead of one set by SetMaxLength
    /// @returns whether the consumer was alive and push succeeded
    /// before the deadline.
    bool PushWithLimitOverride(T&& value, size_t max_len,
                               Deadline deadline = {}) {
      return queue_->Push(std::move(value), deadline, max_len);
    }

    /// Try to push element into queue without blocking.
    /// @returns whether the consumer was alive and push succeeded.
    [[nodiscard]] bool PushNoblock(T&& value) {
      return queue_->PushNoblock(std::move(value));
    }

    /// Const access to source queue.
    std::shared_ptr<const MpscQueue<T>> Queue() const { return {queue_}; }

   private:
    std::shared_ptr<MpscQueue<T>> queue_;
  };

  class Consumer final {
   public:
    Consumer(std::shared_ptr<MpscQueue<T>> queue) : queue_(std::move(queue)) {}

    Consumer(const Consumer&) = delete;

    Consumer(Consumer&&) noexcept = default;

    ~Consumer() {
      if (queue_) queue_->MarkConsumerIsDead();
    }

    /// Pop element from queue. May block if queue is empty, but the producer is
    /// alive.
    /// @returns whether something was popped before the deadline.
    /// @note `false` can be returned before the deadline
    /// when the producer is no longer alive.
    [[nodiscard]] bool Pop(T& value, Deadline deadline = {}) {
      return queue_->Pop(value, deadline);
    }

    /// Try to pop element from queue without blocking.
    /// @return whether something was popped.
    [[nodiscard]] bool PopNoblock(T& value) {
      return queue_->PopNoblock(value);
    }

    /// Const access to source queue.
    std::shared_ptr<const MpscQueue<T>> Queue() const { return {queue_}; }

   private:
    std::shared_ptr<MpscQueue<T>> queue_;
  };

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
  void SetMaxLength(size_t length);

  /// @brief Gets the limit on the queue size
  [[nodiscard]] size_t GetMaxLength() const;

  [[nodiscard]] size_t Size() const;

 private:
  class EmplaceEnabler {};

 public:
  explicit MpscQueue(EmplaceEnabler) {}

 private:
  bool Push(T&&, Deadline, std::optional<size_t>);
  bool PushNoblock(T&&);
  bool DoPush(T&&);

  bool Pop(T&, Deadline);
  bool PopNoblock(T&);
  bool DoPop(T&);

  void MarkConsumerIsDead();

  void MarkProducerIsDead();

  /// Can be called only if Consumer is dead to release all queued items
  bool PopNoblockNoConsumer(T&);

  void WaitForQueueNonfullUntil(Deadline deadline, std::optional<size_t>);

 private:
  // Resolves to boost::lockfree::queue<T> except for std::unique_ptr<T>
  // specialization. In that case, resolves to boost::lockfree::queue<T*>
  typename QueueHelper::LockFreeQueue queue_{1};
  engine::SingleConsumerEvent nonempty_event_;
  engine::Mutex nonfull_mutex_;
  engine::ConditionVariable nonfull_cv_;
#ifndef NDEBUG
  std::atomic<bool> consumer_is_created_{false};
#endif
  std::atomic<bool> consumer_is_created_and_dead_{false};
  std::atomic<bool> producer_is_created_and_dead_{false};
  std::atomic<size_t> producers_count_{0};
  std::atomic<size_t> max_length_{-1UL};
  std::atomic<size_t> size_{0};
};

template <typename T>
std::shared_ptr<MpscQueue<T>> MpscQueue<T>::Create() {
  return std::make_shared<MpscQueue<T>>(EmplaceEnabler{});
}

template <typename T>
MpscQueue<T>::~MpscQueue() {
#ifndef NDEBUG
  UASSERT(consumer_is_created_and_dead_ || !consumer_is_created_);
  UASSERT(!producers_count_);
#endif
  // Clear remaining items in queue. This will work for unique_ptr as well.
  T value;
  while (PopNoblockNoConsumer(value)) {
  }
}

template <typename T>
typename MpscQueue<T>::Producer MpscQueue<T>::GetProducer() {
  this->producers_count_++;
  return Producer(this->shared_from_this());
}

template <typename T>
typename MpscQueue<T>::Consumer MpscQueue<T>::GetConsumer() {
#ifndef NDEBUG
  UASSERT(!this->consumer_is_created_);
  this->consumer_is_created_ = true;
#endif
  return Consumer(this->shared_from_this());
}

template <typename T>
void MpscQueue<T>::SetMaxLength(size_t length) {
  std::unique_lock lock(nonfull_mutex_);
  max_length_ = length;
  // It might result in a spurious wakeup, but it is race-free.
  nonfull_cv_.NotifyAll();
}

template <typename T>
size_t MpscQueue<T>::GetMaxLength() const {
  return max_length_;
}

template <typename T>
size_t MpscQueue<T>::Size() const {
  return size_;
}

template <typename T>
bool MpscQueue<T>::PopNoblockNoConsumer(T& value) {
#ifndef NDEBUG
  UASSERT(this->consumer_is_created_and_dead_ || !this->consumer_is_created_);
#endif
  if (QueueHelper::Pop(queue_, value)) {
    --size_;
    return true;
  }
  return false;
}

template <typename T>
void MpscQueue<T>::WaitForQueueNonfullUntil(Deadline deadline,
                                            std::optional<size_t> max_len) {
  if (size_ >= max_len.value_or(max_length_) &&
      !consumer_is_created_and_dead_) {
    std::unique_lock lock(nonfull_mutex_);
    [[maybe_unused]] auto status =
        nonfull_cv_.WaitUntil(lock, deadline, [this, &max_len] {
          return (size_ < max_len.value_or(max_length_) ||
                  consumer_is_created_and_dead_);
        });
  }
}

template <typename T>
bool MpscQueue<T>::Push(T&& value, Deadline deadline,
                        std::optional<size_t> max_len) {
  WaitForQueueNonfullUntil(deadline, max_len);
  if (deadline.IsReached() || current_task::ShouldCancel()) return false;
  return DoPush(std::move(value));
}

template <typename T>
bool MpscQueue<T>::PushNoblock(T&& value) {
  if (size_ >= max_length_) return false;
  return DoPush(std::move(value));
}

template <typename T>
bool MpscQueue<T>::DoPush(T&& value) {
  if (consumer_is_created_and_dead_) return false;

  ++size_;
  QueueHelper::Push(queue_, std::move(value));
  nonempty_event_.Send();
  return true;
}

template <typename T>
bool MpscQueue<T>::Pop(T& value, Deadline deadline) {
  while (!DoPop(value)) {
    if (producer_is_created_and_dead_ || deadline.IsReached() ||
        current_task::ShouldCancel()) {
      // Producer might have pushed smth in queue between .pop()
      // and !producer_is_created_and_dead_ check. Check twice to avoid
      // TOCTOU.
      return DoPop(value);
    }

    [[maybe_unused]] auto was_nonempty =
        nonempty_event_.WaitForEventUntil(deadline);
  }
  return true;
}

template <typename T>
bool MpscQueue<T>::PopNoblock(T& value) {
  return DoPop(value);
}

template <typename T>
bool MpscQueue<T>::DoPop(T& value) {
  if (QueueHelper::Pop(queue_, value)) {
    std::unique_lock lock(nonfull_mutex_);
    --size_;
    nonfull_cv_.NotifyAll();
    return true;
  }
  return false;
}

template <typename T>
void MpscQueue<T>::MarkConsumerIsDead() {
  consumer_is_created_and_dead_ = true;
  std::unique_lock lock(nonfull_mutex_);
  nonfull_cv_.NotifyAll();
}

template <typename T>
void MpscQueue<T>::MarkProducerIsDead() {
  producer_is_created_and_dead_ = (--producers_count_ == 0);
  nonempty_event_.Send();
}

}  // namespace engine
