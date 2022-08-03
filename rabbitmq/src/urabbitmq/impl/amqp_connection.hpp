#pragma once

#include <memory>

#include <engine/ev/thread_control.hpp>

#include <amqpcpp.h>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::statistics {
class ConnectionStatistics;
}

namespace urabbitmq::impl {

class AmqpConnectionHandler;

class AmqpConnection final {
 public:
  AmqpConnection(AmqpConnectionHandler& handler);
  ~AmqpConnection();

  engine::ev::ThreadControl& GetEvThread();

  AMQP::Connection& GetNative();

  statistics::ConnectionStatistics& GetStatistics();

 private:
  engine::ev::ThreadControl thread_;

  AmqpConnectionHandler& handler_;
  std::unique_ptr<AMQP::Connection> conn_;
};

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END