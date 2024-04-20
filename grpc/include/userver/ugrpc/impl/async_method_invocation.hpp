#pragma once

#include <userver/engine/deadline.hpp>
#include <userver/engine/single_use_event.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

class EventBase {
 public:
  /// @brief For use from the blocking call queue
  /// @param `bool ok` returned by `grpc::CompletionQueue::Next`
  virtual void Notify(bool ok) noexcept = 0;

 protected:
  // One should not call destructor by pointer to interface.
  ~EventBase();
};

class AsyncMethodInvocation : public EventBase {
 public:
  virtual ~AsyncMethodInvocation();

  /// @brief For use from coroutines
  /// @return This object's `void* tag` for `grpc::CompletionQueue::Next`
  void* GetTag() noexcept;

  /// @see EventBase::Notify
  void Notify(bool ok) noexcept override;

  bool IsBusy() const noexcept;

  enum class WaitStatus {
    kOk,
    kError,
    kCancelled,
    // This constant means that engine::Deadline specified for wait operation
    // has expired. Note, that it is not related to the gRPC deadline
    kDeadline,
  };

  /// @brief For use from coroutines
  /// @return `bool ok` returned by `grpc::CompletionQueue::Next`
  [[nodiscard]] WaitStatus Wait() noexcept;

  /// @brief For use from coroutines
  /// @return `bool ok` returned by `grpc::CompletionQueue::Next` or
  ///          information regarding readiness
  [[nodiscard]] WaitStatus WaitUntil(engine::Deadline deadline) noexcept;

  /// @brief Checks if the asynchronous call has completed
  /// @return true if event returned from `grpc::CompletionQueue::Next`
  [[nodiscard]] bool IsReady() const noexcept;

  /// @cond
  // For internal use only.
  engine::impl::ContextAccessor* TryGetContextAccessor() noexcept;
  /// @endcond
 protected:
  void WaitWhileBusy();

 private:
  bool ok_{false};
  bool busy_{false};
  engine::SingleUseEvent event_;
};

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
