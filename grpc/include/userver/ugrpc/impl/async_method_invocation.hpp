#pragma once

#include <userver/engine/single_use_event.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

class EventBase {
 public:
  /// @brief For use from the blocking call queue
  /// @param `bool ok` returned by `grpc::CompletionQueue::Next`
  virtual void Notify(bool ok) noexcept = 0;

  /// @brief For use from coroutines
  /// @returns This object's `void* tag` for `grpc::CompletionQueue::Next`
  void* GetTag() noexcept;

 protected:
  // One should not call destructor by pointer to interface.
  ~EventBase();
};

class AsyncMethodInvocation final : public EventBase {
 public:
  constexpr AsyncMethodInvocation() noexcept = default;

  /// @see EventBase::Notify
  void Notify(bool ok) noexcept override;

  /// @brief For use from coroutines
  /// @returns `bool ok` returned by `grpc::CompletionQueue::Next`
  [[nodiscard]] bool Wait() noexcept;

 private:
  bool ok_{false};
  engine::SingleUseEvent event_;
};

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
