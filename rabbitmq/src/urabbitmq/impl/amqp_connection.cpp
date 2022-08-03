#include "amqp_connection.hpp"

#include <urabbitmq/impl/amqp_connection_handler.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

AmqpConnection::AmqpConnection(AmqpConnectionHandler& handler)
    : thread_{handler.GetEvThread()}, handler_{handler} {
  thread_.RunInEvLoopSync([this] {
    conn_ = std::make_unique<AMQP::Connection>(&handler_);
    handler_.OnConnectionCreated(conn_.get());
  });
}

AmqpConnection::~AmqpConnection() {
  // We can't move connection here, because there might be another operations
  // enqueued in ev-thread
  thread_.RunInEvLoopSync([this] {
    handler_.OnConnectionDestruction();
    conn_.reset();
  });
}

engine::ev::ThreadControl& AmqpConnection::GetEvThread() { return thread_; }

AMQP::Connection& AmqpConnection::GetNative() { return *conn_; }

statistics::ConnectionStatistics& AmqpConnection::GetStatistics() {
  return handler_.GetStatistics();
}

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END
