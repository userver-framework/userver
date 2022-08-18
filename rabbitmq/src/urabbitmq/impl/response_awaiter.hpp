#pragma once

#include <memory>
#include <optional>

#include <userver/engine/semaphore.hpp>
#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

class DeferredWrapper;

class ResponseAwaiter final {
 public:
  ResponseAwaiter(engine::SemaphoreLock&& lock);
  ~ResponseAwaiter();

  ResponseAwaiter(const ResponseAwaiter& other) = delete;
  ResponseAwaiter(ResponseAwaiter&& other) noexcept;

  void SetSpan(tracing::Span&& span);
  void Wait(engine::Deadline deadline) const;

  const std::shared_ptr<DeferredWrapper>& GetWrapper() const;

 private:
#ifndef NDEBUG
  mutable bool awaited_{false};
#endif

  std::optional<tracing::Span> span_;
  engine::SemaphoreLock lock_;
  std::shared_ptr<DeferredWrapper> wrapper_;
};

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END
