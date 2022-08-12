#pragma once

#include <userver/engine/deadline.hpp>
#include <userver/engine/mutex.hpp>

#include <amqpcpp.h>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::statistics {
class ConnectionStatistics;
}

namespace urabbitmq::impl {

class AmqpConnectionHandler;

class AmqpConnection final {
 public:
  AmqpConnection(AmqpConnectionHandler& handler, engine::Deadline deadline);
  ~AmqpConnection();

  AMQP::Connection& GetNative();

  void SetOperationDeadline(engine::Deadline deadline);

  statistics::ConnectionStatistics& GetStatistics();

  class LockGuard final {
   public:
    LockGuard(engine::Mutex& mutex, engine::Deadline deadline);
    ~LockGuard();

   private:
    engine::Mutex& mutex_;
    bool owns_;
  };
  [[nodiscard]] LockGuard Lock(engine::Deadline deadline);

  template <typename Func>
  decltype(auto) RunLocked(Func&& fn, engine::Deadline deadline);

 private:
  AmqpConnectionHandler& handler_;
  AMQP::Connection conn_;
  engine::Mutex mutex_{};
};

template <typename Func>
decltype(auto) AmqpConnection::RunLocked(Func&& fn, engine::Deadline deadline) {
  auto lock = Lock(deadline);
  SetOperationDeadline(deadline);
  return fn();
}

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END