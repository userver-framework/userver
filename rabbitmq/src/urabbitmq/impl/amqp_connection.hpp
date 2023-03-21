#pragma once

#include <userver/engine/deadline.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/semaphore.hpp>

#include <amqpcpp.h>

#include <urabbitmq/impl/amqp_connection_handler.hpp>
#include <urabbitmq/impl/response_awaiter.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::statistics {
class ConnectionStatistics;
}

namespace urabbitmq::impl {

class ConnectionLock final {
 public:
  ConnectionLock(engine::Mutex& mutex, engine::Deadline deadline);
  ~ConnectionLock();

  ConnectionLock(const ConnectionLock& other) = delete;
  ConnectionLock(ConnectionLock&& other) noexcept;

 private:
  engine::Mutex& mutex_;
  bool owns_;
};

template <typename Channel>
class LockedChannelProxy final {
 public:
  LockedChannelProxy(Channel& channel, ConnectionLock&& lock)
      : channel_{channel}, lock_{std::move(lock)} {}
  ~LockedChannelProxy() = default;

  LockedChannelProxy(const LockedChannelProxy& other) = delete;
  LockedChannelProxy(LockedChannelProxy&& other) = delete;

  Channel* operator->() { return &channel_; }

 private:
  Channel& channel_;
  ConnectionLock lock_;
};

class AmqpConnection final {
 public:
  AmqpConnection(AmqpConnectionHandler& handler, size_t max_in_flight_requests,
                 engine::Deadline deadline);
  ~AmqpConnection();

  AMQP::Connection& GetNative();

  void SetOperationDeadline(engine::Deadline deadline);

  statistics::ConnectionStatistics& GetStatistics();

  LockedChannelProxy<AMQP::Channel> GetChannel(engine::Deadline deadline);

  using ReliableChannel = AMQP::Reliable<AMQP::Tagger>;
  LockedChannelProxy<ReliableChannel> GetReliableChannel(
      engine::Deadline deadline);

  ResponseAwaiter GetAwaiter(engine::Deadline deadline);

 private:
  friend class AmqpConnectionLocker;
  [[nodiscard]] ConnectionLock Lock(engine::Deadline deadline);

  AMQP::Channel CreateChannel(engine::Deadline deadline);
  std::unique_ptr<ReliableChannel> CreateReliable(engine::Deadline deadline);

  void AwaitChannelCreated(AMQP::Channel& channel, engine::Deadline deadline);

  template <typename Channel>
  LockedChannelProxy<Channel> DoGetChannel(Channel& channel,
                                           engine::Deadline deadline) {
    auto lock = Lock(deadline);
    if (handler_.IsBroken()) {
      throw std::runtime_error{"Connection is broken"};
    }
    handler_.SetOperationDeadline(deadline);
    return {channel, std::move(lock)};
  }

  AmqpConnectionHandler& handler_;

  AMQP::Connection conn_;

  AMQP::Channel channel_;

  AMQP::Channel reliable_channel_;
  std::unique_ptr<ReliableChannel> reliable_;

  // This can't be SingleWaitingTaskMutex, because consumers might issue a lot
  // of ack/nack in parallel.
  engine::Mutex mutex_{};
  engine::Semaphore waiters_sema_;
};

class AmqpConnectionLocker final {
 public:
  AmqpConnectionLocker(AmqpConnection& conn);

  [[nodiscard]] ConnectionLock Lock(engine::Deadline deadline);

 private:
  AmqpConnection& conn_;
};

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END
