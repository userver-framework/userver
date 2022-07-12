#pragma once

#include <memory>

#include <boost/lockfree/queue.hpp>

#include <userver/clients/dns/resolver_fwd.hpp>

#include <urabbitmq/channel_ptr.hpp>
#include <urabbitmq/impl/amqp_connection.hpp>
#include <urabbitmq/impl/amqp_connection_handler.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

struct EndpointInfo;
struct AuthSettings;

enum class ConnectionMode {
  // Channels created with this mode are not reliable
  kUnreliable,
  // Channels created with this mode are put in "confirm" mode,
  // i.e. PublisherConfirms
  kReliable
};

struct ConnectionSettings final {
  ConnectionMode mode;
  size_t max_channels;
};

class Connection final : public std::enable_shared_from_this<Connection> {
 public:
  Connection(clients::dns::Resolver& resolver,
             engine::ev::ThreadControl& thread,
             const ConnectionSettings& settings, const EndpointInfo& endpoint,
             const AuthSettings& auth_settings);
  ~Connection();

  ChannelPtr Acquire();
  void Release(impl::IAmqpChannel* channel) noexcept;

  bool IsBroken() const;

 private:
  impl::IAmqpChannel* Pop();
  impl::IAmqpChannel* TryPop();

  std::unique_ptr<impl::IAmqpChannel> CreateChannel();
  static void Drop(impl::IAmqpChannel* channel);

  void AddChannel();

  impl::AmqpConnectionHandler handler_;
  impl::AmqpConnection conn_;

  const ConnectionSettings settings_;
  boost::lockfree::queue<impl::IAmqpChannel*> queue_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END