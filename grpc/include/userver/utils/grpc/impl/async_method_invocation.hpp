#pragma once

#include <userver/engine/single_use_event.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::grpc::impl {

class AsyncMethodInvocation final {
 public:
  constexpr AsyncMethodInvocation() noexcept = default;

  /// @brief For use from the blocking call queue
  /// @param `bool ok` returned by `::grpc::CompletionQueue::Next`
  void Notify(bool ok) noexcept;

  /// @brief For use from coroutines
  /// @returns This object's `void* tag` for `::grpc::CompletionQueue::Next`
  void* GetTag() noexcept;

  /// @brief For use from coroutines
  /// @returns `bool ok` returned by `::grpc::CompletionQueue::Next`
  [[nodiscard]] bool Wait() noexcept;

 private:
  bool ok_{false};
  engine::SingleUseEvent event_;
};

}  // namespace utils::grpc::impl

USERVER_NAMESPACE_END
