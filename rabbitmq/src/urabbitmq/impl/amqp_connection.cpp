#include "amqp_connection.hpp"

#include <urabbitmq/impl/amqp_connection_handler.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

AmqpConnection::AmqpConnection(AmqpConnectionHandler& handler) : thread_{handler.GetEvThread()}, handler_{handler} {
  thread_.RunInEvLoopSync([this] {
    conn_ = std::make_unique<AMQP::Connection>(&handler_);
    handler_.OnConnectionCreated(conn_.get());
  });
}

AmqpConnection::~AmqpConnection() {
  thread_.RunInEvLoopSync([this, conn = std::move(conn_)] () mutable {
    handler_.OnConnectionDestruction();
    conn.reset();
  });
}

engine::ev::ThreadControl& AmqpConnection::GetEvThread() {
  return thread_;
}

AMQP::Connection& AmqpConnection::GetNative() {
  return *conn_;
}

}

USERVER_NAMESPACE_END
