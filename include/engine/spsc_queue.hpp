#pragma once

#include <boost/lockfree/queue.hpp>

#include <engine/single_consumer_event.hpp>

namespace engine {

/* Single producer, single consumer queue */
template <typename T>
class SpscQueue : public std::enable_shared_from_this<SpscQueue<T>> {
 public:
  virtual ~SpscQueue();

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

    /// Push element into queue. May block if queue is full.
    /// Returns false if consumer is dead.
    bool Push(T&& value) { return queue_->Push(std::move(value)); }

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

    /// Pop element. Returns true if smth is popped, false if queue is empty,
    /// producer is dead, and no element is popped.
    /// May block if queue is empty, but producer is alive.
    bool Pop(T& value) { return queue_->Pop(value); }

   private:
    std::shared_ptr<SpscQueue<T>> queue_;
  };

  /// Can be called only once
  Producer GetProducer();

  /// Can be called only once
  Consumer GetConsumer();

  void SetMaxLength(size_t length);

  size_t Size() const;

  /// Can be called only if Consumer is dead to release all queued items
  bool PopNoblockNoConsumer(T&);

 protected:
  SpscQueue()
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

  bool Pop(T&);

  bool PopNoblock(T&);

  void MarkConsumerIsDead();

  void MarkProducerIsDead();

  friend class Producer;
  friend class Consumer;

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

  struct X : SpscQueue<T> {
    X() = default;
  };
  return std::make_shared<X>();
}

template <typename T>
SpscQueue<T>::~SpscQueue() {
  assert(!consumer_is_alive_ || !consumer_is_created_);
  assert(!producer_is_alive_ || !producer_is_created_);
}

template <typename T>
typename SpscQueue<T>::Producer SpscQueue<T>::GetProducer() {
  assert(!this->producer_is_created_);
#ifndef NDEBUG
  this->producer_is_created_ = true;
#endif
  return Producer(this->shared_from_this());
}

template <typename T>
typename SpscQueue<T>::Consumer SpscQueue<T>::GetConsumer() {
  assert(!this->consumer_is_created_);
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
  assert(!this->consumer_is_alive_ || !this->consumer_is_created_);
  return this->PopNoblock(value);
}

template <typename T>
bool SpscQueue<T>::Push(T&& value) {
  while (size_ >= max_length_) {
    if (!consumer_is_alive_) return false;

    nonfull_event_.WaitForEvent();
  }

  if (!consumer_is_alive_) return false;

  ++size_;
  queue_.push(std::move(value));
  nonempty_event_.Send();
  return true;
}

template <typename T>
bool SpscQueue<T>::Pop(T& value) {
  while (!PopNoblock(value)) {
    if (!producer_is_alive_) {
      // Producer might have pushed smth in queue between .pop()
      // and !producer_is_alive_ check. Check twice to avoid TOCTOU.
      if (PopNoblock(value)) break;
      return false;
    }

    nonempty_event_.WaitForEvent();
  }

  // queue_.pop() is ok
  --size_;
  nonfull_event_.Send();
  return true;
}

template <typename T>
bool SpscQueue<T>::PopNoblock(T& value) {
  return queue_.pop(value);
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

/* Template specialization for unique_ptr<T>: it queues T* and calls T::delete
 * for all non-pop'ed items in destructor.
 */
template <typename T>
class SpscQueue<std::unique_ptr<T>> : public SpscQueue<T*> {
 public:
  static std::shared_ptr<SpscQueue<std::unique_ptr<T>>> Create() {
    struct X : SpscQueue<std::unique_ptr<T>> {
      X() = default;
    };

    return std::make_shared<X>();
  }

  ~SpscQueue() noexcept {
    T* value;
    while (this->PopNoblockNoConsumer(value)) {
      delete value;
    }
  }

  using typename SpscQueue<T*>::Producer;

  class Consumer {
   public:
    Consumer(typename SpscQueue<T*>::Consumer consumer)
        : consumer_(std::move(consumer)) {}

    Consumer(const Consumer&) = delete;

    Consumer(Consumer&&) = default;

    bool Pop(std::unique_ptr<T>& value) {
      T* ptr;
      if (!consumer_.Pop(ptr)) return false;
      value.reset(ptr);
      return true;
    }

   private:
    typename SpscQueue<T*>::Consumer consumer_;
  };

  Consumer GetConsumer() {
    return Consumer(this->SpscQueue<T*>::GetConsumer());
  }

  using SpscQueue<T*>::GetProducer;
  using SpscQueue<T*>::Size;
  using SpscQueue<T*>::SetMaxLength;

 private:
  SpscQueue() = default;
};

}  // namespace engine
