#pragma once

#include <atomic>
#include <memory>

#include <boost/lockfree/queue.hpp>

#include <engine/single_consumer_event.hpp>
#include <engine/task/cancel.hpp>
#include <utils/assert.hpp>

namespace engine {

/// Single producer, single consumer queue
template <typename T>
class SpscQueue final : public std::enable_shared_from_this<SpscQueue<T>> {
 public:
  ~SpscQueue();

  SpscQueue(const SpscQueue&) = delete;

  SpscQueue(SpscQueue&&) = delete;

  static std::shared_ptr<SpscQueue> Create();

  class Producer {
   public:
    Producer(std::shared_ptr<SpscQueue<T>> queue) : queue_(std::move(queue)) {}

    Producer(const Producer&) = delete;

    Producer(Producer&&) = default;

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

   private:
    std::shared_ptr<SpscQueue<T>> queue_;
  };

  class Consumer {
   public:
    Consumer(std::shared_ptr<SpscQueue<T>> queue) : queue_(std::move(queue)) {}

    Consumer(const Consumer&) = delete;

    Consumer(Consumer&&) = default;

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

  template <typename>
  friend class SpscQueue;

 private:
  boost::lockfree::queue<T> queue_;
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
  static_assert(boost::has_trivial_destructor<T>::value,
                "T has non-trivial destructor. Use "
                "SpscQueue<std::unique_ptr<T>> instead of SpscQueue<T> or use "
                "another T.");

  return std::make_shared<SpscQueue<T>>(EmplaceEnabler{});
}

template <typename T>
SpscQueue<T>::~SpscQueue() {
  UASSERT(!consumer_is_alive_ || !consumer_is_created_);
  UASSERT(!producer_is_alive_ || !producer_is_created_);
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
size_t SpscQueue<T>::Size() const {
  return size_;
}

template <typename T>
bool SpscQueue<T>::PopNoblockNoConsumer(T& value) {
  UASSERT(!this->consumer_is_alive_ || !this->consumer_is_created_);
  if (queue_.pop(value)) {
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
  queue_.push(std::move(value));
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
  if (queue_.pop(value)) {
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

/// Template specialization for `unique_ptr<T>`: it queues `T*` and calls
/// `T::delete` for all non-poped items in destructor.
template <typename T>
class SpscQueue<std::unique_ptr<T>> final {
 public:
  SpscQueue()
      : impl_(std::make_shared<SpscQueue<T*>>(
            typename SpscQueue<T*>::EmplaceEnabler{})) {}

  static auto Create() {
    return std::make_shared<SpscQueue<std::unique_ptr<T>>>();
  }

  ~SpscQueue() noexcept {
    T* value;
    while (impl_->PopNoblockNoConsumer(value)) {
      delete value;
    }
  }

  class Consumer {
   public:
    Consumer(typename SpscQueue<T*>::Consumer consumer)
        : consumer_(std::move(consumer)) {}

    Consumer(const Consumer&) = delete;

    Consumer(Consumer&&) = default;

    [[nodiscard]] bool Pop(std::unique_ptr<T>& value) {
      T* ptr;
      if (!consumer_.Pop(ptr)) return false;
      value.reset(ptr);
      return true;
    }

    [[nodiscard]] bool PopNoblock(std::unique_ptr<T>& value) {
      T* ptr;
      if (!consumer_.PopNoblock(ptr)) return false;
      value.reset(ptr);
      return true;
    }

   private:
    typename SpscQueue<T*>::Consumer consumer_;
  };

  class Producer {
   public:
    Producer(typename SpscQueue<T*>::Producer producer)
        : producer_(std::move(producer)) {}

    Producer(const Producer&) = delete;

    Producer(Producer&&) = default;

    bool Push(std::unique_ptr<T>&& value) {
      if (!producer_.Push(value.get())) return false;
      value.release();
      return true;
    }

    bool PushNoblock(std::unique_ptr<T>&& value) {
      if (!producer_.PushNoblock(value.get())) return false;
      value.release();
      return true;
    }

   private:
    typename SpscQueue<T*>::Producer producer_;
  };

  Consumer GetConsumer() { return Consumer(impl_->GetConsumer()); }

  Producer GetProducer() { return Producer(impl_->GetProducer()); }

  size_t Size() const { return impl_->Size(); }
  void SetMaxLength(size_t length) { impl_->SetMaxLength(length); }

 private:
  std::shared_ptr<SpscQueue<T*>> impl_;
};

}  // namespace engine
