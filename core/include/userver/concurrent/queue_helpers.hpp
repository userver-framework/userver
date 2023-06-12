#pragma once

#include <memory>

#include <userver/engine/deadline.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

namespace impl {

struct NoToken final {
  template <typename LockFreeQueue>
  explicit NoToken(LockFreeQueue& /*unused*/) {}
};

}  // namespace impl

/// @warning A single Producer must not be used from multiple threads
/// concurrently
template <typename QueueType, typename ProducerToken,
          typename EmplaceEnablerType>
class Producer final {
  static_assert(
      std::is_same_v<EmplaceEnablerType, typename QueueType::EmplaceEnabler>,
      "Do not instantiate Producer on your own. Use Producer type alias "
      "from queue");

  using ValueType = typename QueueType::ValueType;

 public:
  Producer(const Producer&) = delete;
  Producer(Producer&&) noexcept = default;
  Producer& operator=(const Producer&) = delete;
  Producer& operator=(Producer&&) noexcept = default;

  ~Producer() {
    if (queue_) queue_->MarkProducerIsDead();
  }

  /// Push element into queue. May wait asynchronously if the queue is full.
  /// Leaves the `value` unmodified if the operation does not succeed.
  /// @returns whether push succeeded before the deadline and before the task
  /// was canceled.
  [[nodiscard]] bool Push(ValueType&& value,
                          engine::Deadline deadline = {}) const {
    UASSERT(queue_);
    return queue_->Push(token_, std::move(value), deadline);
  }

  /// Try to push element into queue without blocking. May be used in
  /// non-coroutine environment. Leaves the `value` unmodified if the operation
  /// does not succeed.
  /// @returns whether push succeeded.
  [[nodiscard]] bool PushNoblock(ValueType&& value) const {
    UASSERT(queue_);
    return queue_->PushNoblock(token_, std::move(value));
  }

  void Reset() && {
    if (queue_) queue_->MarkProducerIsDead();
    queue_.reset();
  }

  /// Const access to source queue.
  [[nodiscard]] std::shared_ptr<const QueueType> Queue() const {
    return {queue_};
  }

  /// @cond
  // For internal use only
  Producer(std::shared_ptr<QueueType> queue, EmplaceEnablerType /*unused*/)
      : queue_(std::move(queue)), token_(queue_->queue_) {}
  /// @endcond

 private:
  std::shared_ptr<QueueType> queue_;
  mutable ProducerToken token_;
};

/// @warning A single Consumer must not be used from multiple threads
/// concurrently
template <typename QueueType, typename EmplaceEnablerType>
class Consumer final {
  static_assert(
      std::is_same_v<EmplaceEnablerType, typename QueueType::EmplaceEnabler>,
      "Do not instantiate Consumer on your own. Use Consumer type alias "
      "from queue");

  using ValueType = typename QueueType::ValueType;
  using ConsumerToken = typename QueueType::ConsumerToken;

 public:
  Consumer(const Consumer&) = delete;
  Consumer(Consumer&&) noexcept = default;
  Consumer& operator=(const Consumer&) = delete;
  Consumer& operator=(Consumer&&) noexcept = default;

  ~Consumer() {
    if (queue_) queue_->MarkConsumerIsDead();
  }

  /// Pop element from queue. May wait asynchronously if the queue is empty,
  /// but the producer is alive.
  /// @returns whether something was popped before the deadline.
  /// @note `false` can be returned before the deadline
  /// when the producer is no longer alive.
  [[nodiscard]] bool Pop(ValueType& value,
                         engine::Deadline deadline = {}) const {
    return queue_->Pop(token_, value, deadline);
  }

  /// Try to pop element from queue without blocking. May be used in
  /// non-coroutine environment
  /// @return whether something was popped.
  [[nodiscard]] bool PopNoblock(ValueType& value) const {
    return queue_->PopNoblock(token_, value);
  }

  /// Const access to source queue.
  [[nodiscard]] std::shared_ptr<const QueueType> Queue() const {
    return {queue_};
  }

  /// @cond
  // For internal use only
  Consumer(std::shared_ptr<QueueType> queue, EmplaceEnablerType /*unused*/)
      : queue_(std::move(queue)), token_(queue_->queue_) {}
  /// @endcond

 private:
  std::shared_ptr<QueueType> queue_{};
  mutable ConsumerToken token_;
};

}  // namespace concurrent

USERVER_NAMESPACE_END
