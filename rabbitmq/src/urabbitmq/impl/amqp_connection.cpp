#include "amqp_connection.hpp"

#include <urabbitmq/impl/amqp_connection_handler.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

AmqpConnection::AmqpConnection(AmqpConnectionHandler& handler)
    : handler_{handler},
      // TODO : deadline?
      conn_{&handler} {
  handler_.OnConnectionCreated(this);
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
