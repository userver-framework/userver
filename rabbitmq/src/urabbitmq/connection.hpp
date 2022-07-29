#pragma once

#include <memory>

#include <boost/lockfree/queue.hpp>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/utils/periodic_task.hpp>

#include <urabbitmq/channel_ptr.hpp>
#include <urabbitmq/impl/amqp_connection.hpp>
#include <urabbitmq/impl/amqp_connection_handler.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

struct EndpointInfo;
struct AuthSettings;

class ChannelPool;

class Connection final {
 public:
  Connection(clients::dns::Resolver& resolver,
             engine::ev::ThreadControl& thread, const EndpointInfo& endpoint,
             const AuthSettings& auth_settings, bool secure,
             size_t max_channels);
  ~Connection();

  ChannelPtr Acquire() const;
  ChannelPtr AcquireReliable() const;

  bool IsBroken() const;

 private:
  impl::AmqpConnectionHandler handler_;
  impl::AmqpConnection connection_;

  std::shared_ptr<ChannelPool> channels_;
  std::shared_ptr<ChannelPool> reliable_channels_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END