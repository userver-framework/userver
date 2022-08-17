#pragma once

#include <memory>
#include <optional>
#include <string>

#include <userver/engine/semaphore.hpp>
#include <userver/engine/single_consumer_event.hpp>

namespace AMQP {
class Deferred;
}

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

class AmqpConnection;

class DeferredWrapper : public std::enable_shared_from_this<DeferredWrapper> {
 public:
  void Fail(const char* message);

  void Ok();

  void Wait(engine::Deadline deadline);

  void Wrap(AMQP::Deferred& deferred);

  static std::shared_ptr<DeferredWrapper> Create();

 protected:
  DeferredWrapper();

 private:
  std::atomic<bool> is_signaled_{false};
  engine::SingleConsumerEvent event_;
  std::optional<std::string> error_;
};

class ResponseAwaiter final {
 public:
  ResponseAwaiter(engine::SemaphoreLock&& lock);
  ~ResponseAwaiter();

  ResponseAwaiter(const ResponseAwaiter& other) = delete;
  ResponseAwaiter(ResponseAwaiter&& other) noexcept;

  void Wait(engine::Deadline deadline) const;

  const std::shared_ptr<DeferredWrapper>& GetWrapper() const;

 private:
  engine::SemaphoreLock lock_;
  std::shared_ptr<DeferredWrapper> wrapper_;
};

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END