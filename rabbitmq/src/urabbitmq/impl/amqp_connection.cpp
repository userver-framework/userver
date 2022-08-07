#include "amqp_connection.hpp"

#include <urabbitmq/impl/amqp_connection_handler.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

namespace {

AMQP::Connection CreateConnection(AmqpConnectionHandler& handler,
                                  engine::Deadline deadline) {
  handler.SetOperationDeadline(deadline);
  return {&handler};
}

}  // namespace

AmqpConnection::AmqpConnection(AmqpConnectionHandler& handler,
                               engine::Deadline deadline)
    : handler_{handler}, conn_{CreateConnection(handler_, deadline)} {
  handler_.OnConnectionCreated(this, deadline);
}

AmqpConnection::~AmqpConnection() { handler_.OnConnectionDestruction(); }

AMQP::Connection& AmqpConnection::GetNative() { return conn_; }

void AmqpConnection::SetOperationDeadline(engine::Deadline deadline) {
  handler_.SetOperationDeadline(deadline);
}

statistics::ConnectionStatistics& AmqpConnection::GetStatistics() {
  return handler_.GetStatistics();
}

AmqpConnection::LockGuard::LockGuard(engine::Mutex& mutex,
                                     engine::Deadline deadline)
    : mutex_{mutex}, owns_{mutex_.try_lock_until(deadline)} {
  if (!owns_) {
    throw std::runtime_error{"deadline idk"};
  }
}

AmqpConnection::LockGuard::~LockGuard() {
  if (owns_) {
    mutex_.unlock();
  }
}

AmqpConnection::LockGuard AmqpConnection::Lock(engine::Deadline deadline) {
  return {mutex_, deadline};
}

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END
