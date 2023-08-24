#pragma once

#include <memory>

#include <boost/lockfree/queue.hpp>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/utils/periodic_task.hpp>

#include <urabbitmq/impl/amqp_channel.hpp>
#include <urabbitmq/impl/amqp_connection.hpp>
#include <urabbitmq/impl/amqp_connection_handler.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

struct EndpointInfo;
struct AuthSettings;

namespace statistics {
class ConnectionStatistics;
}

class Connection final {
 public:
  Connection(clients::dns::Resolver& resolver, const EndpointInfo& endpoint,
             const AuthSettings& auth_settings, size_t max_in_flight_requests,
             bool secure, statistics::ConnectionStatistics& stats,
             engine::Deadline deadline);
  ~Connection();

  impl::AmqpChannel& GetChannel();
  impl::AmqpReliableChannel& GetReliableChannel();

  bool IsBroken() const;

  void EnsureUsable() const;

 private:
  impl::AmqpConnectionHandler handler_;
  impl::AmqpConnection connection_;

  impl::AmqpChannel channel_;
  impl::AmqpReliableChannel reliable_channel_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
