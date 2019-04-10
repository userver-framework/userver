#pragma once

#include <atomic>
#include <memory>

#include <boost/lockfree/queue.hpp>

#include <engine/single_consumer_event.hpp>
#include <engine/task/cancel.hpp>
#include <utils/assert.hpp>

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
                "SpscQueue<std::unique_ptr<T>> instead of SpscQueue<T> or use "
                "another T.");
};

/// This partial specialization is helper's raison d'être. It allows
/// one to pass std::unique_ptr via SpscQueue
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

/// Single producer, single consumer queue
template <typename T>
class SpscQueue final : public std::enable_shared_from_this<SpscQueue<T>> {
  using QueueHelper = impl::QueueHelper<T>;

 public:
  ~SpscQueue();

  SpscQueue(const SpscQueue&) = delete;

  SpscQueue(SpscQueue&&) = delete;

  static std::shared_ptr<SpscQueue> Create();

  class Producer {
   public:
    Producer(std::shared_ptr<SpscQueue<T>> queue) : queue_(std::move(queue)) {}

    Producer(const Producer&) = delete;

    Producer(Producer&&) noexcept = default;

    ~Producer() {
      if (queue_) queue_->MarkProducerIsDead();
    }

    /// Push element into queue. May block if queue is full, but the consumer is
    /// alive.
    /// @returns whether the consumer was alive and push succeeded.
    bool Push(T&& value) { return queue_->Push(std::move(value)); }

    /// Try to push element into queue without blocking.
    /// @returns whether the consumer was alive and push succeeded.
    [[nodiscard]] bool PushNoblock(T&& value) {
      return queue_->PushNoblock(std::move(value));
    }

    /// Const access to source queue.
    std::shared_ptr<const SpscQueue<T>> Queue() const { return {queue_}; }

   private:
    std::shared_ptr<SpscQueue<T>> queue_;
  };

  class Consumer {
   public:
    Consumer(std::shared_ptr<SpscQueue<T>> queue) : queue_(std::move(queue)) {}

    Consumer(const Consumer&) = delete;

    Consumer(Consumer&&) noexcept = default;

    ~Consumer() {
      if (queue_) queue_->MarkConsumerIsDead();
    }

    /// Pop element from queue. May block if queue is empty, but the producer is
    /// alive.
    /// @returns whether something was popped.
    /// @note `false` is returned only when the producer is no longer alive.
    [[nodiscard]] bool Pop(T& value) { return queue_->Pop(value); }

    /// Try to pop element from queue without blocking.
    /// @return whether something was popped.
    [[nodiscard]] bool PopNoblock(T& value) {
      return queue_->PopNoblock(value);
    }

    /// Const access to source queue.
    std::shared_ptr<const SpscQueue<T>> Queue() const { return {queue_}; }

   private:
    std::shared_ptr<SpscQueue<T>> queue_;
  };

  /// Can be called only once
  Producer GetProducer();

  /// Can be called only once
  Consumer GetConsumer();

  /// @brief Sets the limit on the queue size, pushes over this limit will block
  /// @note This is a soft limit and may be slightly overrun under load.
  void SetMaxLength(size_t length);

  /// @brief Gets the limit on the queue size
  size_t GetMaxLength() const;

  size_t Size() const;

 private:
  class EmplaceEnabler {};

 public:
  explicit SpscQueue(EmplaceEnabler)
      :
#ifndef NDEBUG
        consumer_is_created_{false},
        producer_is_created_{false},
#endif
        consumer_is_alive_{true},
        producer_is_alive_{true},
        max_length_{-1UL},
        size_{0} {
  }

 private:
  bool Push(T&&);
  bool PushNoblock(T&&);
  bool DoPush(T&&);

  bool Pop(T&);
  bool PopNoblock(T&);
  bool DoPop(T&);

  void MarkConsumerIsDead();

  void MarkProducerIsDead();

  /// Can be called only if Consumer is dead to release all queued items
  bool PopNoblockNoConsumer(T&);

 private:
  // Resolves to boost::lockfree::queue<T> except for std::unique_ptr<T>
  // specialization. In that case, resolves to boost::lockfree::queue<T*>
  typename QueueHelper::LockFreeQueue queue_;
  engine::SingleConsumerEvent nonempty_event_, nonfull_event_;
#ifndef NDEBUG
  std::atomic<bool> consumer_is_created_, producer_is_created_;
#endif
  std::atomic<bool> consumer_is_alive_, producer_is_alive_;
  std::atomic<size_t> max_length_;
  std::atomic<size_t> size_;
};

template <typename T>
std::shared_ptr<SpscQueue<T>> SpscQueue<T>::Create() {
  return std::make_shared<SpscQueue<T>>(EmplaceEnabler{});
}

template <typename T>
SpscQueue<T>::~SpscQueue() {
  UASSERT(!consumer_is_alive_ || !consumer_is_created_);
  UASSERT(!producer_is_alive_ || !producer_is_created_);
  // Clear remaining items in queue. This will work for unique_ptr as well.
  T value;
  while (PopNoblockNoConsumer(value)) {
  }
}

template <typename T>
typename SpscQueue<T>::Producer SpscQueue<T>::GetProducer() {
  UASSERT(!this->producer_is_created_);
#ifndef NDEBUG
  this->producer_is_created_ = true;
#endif
  return Producer(this->shared_from_this());
}

template <typename T>
typename SpscQueue<T>::Consumer SpscQueue<T>::GetConsumer() {
  UASSERT(!this->consumer_is_created_);
#ifndef NDEBUG
  this->consumer_is_created_ = true;
#endif
  return Consumer(this->shared_from_this());
}

template <typename T>
void SpscQueue<T>::SetMaxLength(size_t length) {
  max_length_ = length;
  // It might result in a spurious wakeup, but it is race-free.
  nonfull_event_.Send();
}

template <typename T>
size_t SpscQueue<T>::GetMaxLength() const {
  return max_length_;
}

template <typename T>
size_t SpscQueue<T>::Size() const {
  return size_;
}

template <typename T>
bool SpscQueue<T>::PopNoblockNoConsumer(T& value) {
  UASSERT(!this->consumer_is_alive_ || !this->consumer_is_created_);
  if (QueueHelper::Pop(queue_, value)) {
    --size_;
    return true;
  }
  return false;
}

template <typename T>
bool SpscQueue<T>::Push(T&& value) {
  while (size_ >= max_length_ && consumer_is_alive_) {
    if (current_task::ShouldCancel()) return false;

    [[maybe_unused]] auto was_nonfull = nonfull_event_.WaitForEvent();
  }
  return DoPush(std::move(value));
}

template <typename T>
bool SpscQueue<T>::PushNoblock(T&& value) {
  if (size_ >= max_length_) return false;
  return DoPush(std::move(value));
}

template <typename T>
bool SpscQueue<T>::DoPush(T&& value) {
  if (!consumer_is_alive_) return false;

  ++size_;
  QueueHelper::Push(queue_, std::move(value));
  nonempty_event_.Send();
  return true;
}

template <typename T>
bool SpscQueue<T>::Pop(T& value) {
  while (!DoPop(value)) {
    if (!producer_is_alive_ || current_task::ShouldCancel()) {
      // Producer might have pushed smth in queue between .pop()
      // and !producer_is_alive_ check. Check twice to avoid TOCTOU.
      return DoPop(value);
    }

    [[maybe_unused]] auto was_nonempty = nonempty_event_.WaitForEvent();
  }
  return true;
}

template <typename T>
bool SpscQueue<T>::PopNoblock(T& value) {
  return DoPop(value);
}

template <typename T>
bool SpscQueue<T>::DoPop(T& value) {
  if (QueueHelper::Pop(queue_, value)) {
    --size_;
    nonfull_event_.Send();
    return true;
  }
  return false;
}

template <typename T>
void SpscQueue<T>::MarkConsumerIsDead() {
  consumer_is_alive_ = false;
  nonfull_event_.Send();
}

template <typename T>
void SpscQueue<T>::MarkProducerIsDead() {
  producer_is_alive_ = false;
  nonempty_event_.Send();
}

}  // namespace engine
