#include "amqp_connection.hpp"

#include <urabbitmq/impl/amqp_connection_handler.hpp>
#include <urabbitmq/impl/deferred_wrapper.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

namespace {

constexpr std::chrono::milliseconds kGracefulCloseTimeout{1000};

AMQP::Connection CreateConnection(AmqpConnectionHandler& handler,
                                  engine::Deadline deadline) {
  const auto& address = handler.GetAddress();
  handler.SetOperationDeadline(deadline);
  return {&handler, address.login(), address.vhost()};
}

}  // namespace

ConnectionLock::ConnectionLock(engine::Mutex& mutex, engine::Deadline deadline)
    : mutex_{mutex}, owns_{mutex_.try_lock_until(deadline)} {
  if (!owns_) {
    throw std::runtime_error{
        "Failed to acquire a connection within specified deadline"};
  }
}

ConnectionLock::~ConnectionLock() {
  if (owns_) {
    mutex_.unlock();
  }
}

ConnectionLock::ConnectionLock(ConnectionLock&& other) noexcept
    : mutex_{other.mutex_}, owns_{std::exchange(other.owns_, false)} {}

AmqpConnection::AmqpConnection(AmqpConnectionHandler& handler,
                               size_t max_in_flight_requests,
                               engine::Deadline deadline)
    : handler_{handler},
      conn_{CreateConnection(handler_, deadline)},
      channel_{CreateChannel(deadline)},
      reliable_channel_{CreateChannel(deadline)},
      waiters_sema_{max_in_flight_requests} {
  handler_.OnConnectionCreated(this, deadline);

  try {
    AwaitChannelCreated(channel_, deadline);
    AwaitChannelCreated(reliable_channel_, deadline);

    reliable_ = CreateReliable(deadline);
  } catch (const std::exception&) {
    handler_.OnConnectionDestruction();
    throw;
  }
}

AmqpConnection::~AmqpConnection() {
  handler_.OnConnectionDestruction();
  handler_.SetOperationDeadline(
      engine::Deadline::FromDuration(kGracefulCloseTimeout));
}

AMQP::Connection& AmqpConnection::GetNative() { return conn_; }

void AmqpConnection::SetOperationDeadline(engine::Deadline deadline) {
  handler_.SetOperationDeadline(deadline);
}

statistics::ConnectionStatistics& AmqpConnection::GetStatistics() {
  return handler_.GetStatistics();
}

LockedChannelProxy<AMQP::Channel> AmqpConnection::GetChannel(
    engine::Deadline deadline) {
  return DoGetChannel(channel_, deadline);
}

LockedChannelProxy<AmqpConnection::ReliableChannel>
AmqpConnection::GetReliableChannel(engine::Deadline deadline) {
  return DoGetChannel(*reliable_, deadline);
}

ResponseAwaiter AmqpConnection::GetAwaiter(engine::Deadline deadline) {
  engine::SemaphoreLock lock{waiters_sema_, deadline};
  if (!lock.OwnsLock()) {
    throw std::runtime_error{
        "Failed to acquire a connection within specified deadline"};
  }

  return ResponseAwaiter{std::move(lock)};
}

ConnectionLock AmqpConnection::Lock(engine::Deadline deadline) {
  return {mutex_, deadline};
}

AMQP::Channel AmqpConnection::CreateChannel(engine::Deadline deadline) {
  auto lock = Lock(deadline);
  SetOperationDeadline(deadline);
  return {&GetNative()};
}

std::unique_ptr<AmqpConnection::ReliableChannel> AmqpConnection::CreateReliable(
    engine::Deadline deadline) {
  auto lock = Lock(deadline);
  SetOperationDeadline(deadline);
  return std::make_unique<AmqpConnection::ReliableChannel>(reliable_channel_);
}

void AmqpConnection::AwaitChannelCreated(AMQP::Channel& channel,
                                         engine::Deadline deadline) {
  auto deferred = DeferredWrapper::Create();

  {
    auto lock = Lock(deadline);
    channel.onReady([deferred] { deferred->Ok(); });
    channel.onError([deferred](const char* error) { deferred->Fail(error); });
  }

  deferred->Wait(deadline);
}

AmqpConnectionLocker::AmqpConnectionLocker(AmqpConnection& conn)
    : conn_{conn} {}

ConnectionLock AmqpConnectionLocker::Lock(engine::Deadline deadline) {
  return conn_.Lock(deadline);
}

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END
