#pragma once

#include <memory>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/utils/fast_pimpl.hpp>

#include <userver/urabbitmq/client_settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class AdminChannel;
class Channel;
class ReliableChannel;
class ConsumerBase;
class ClientImpl;

/// Interface for communicating with a RabbitMQ cluster.
class Client : public std::enable_shared_from_this<Client> {
 public:
  static std::shared_ptr<Client> Create(clients::dns::Resolver& resolver,
                                        const ClientSettings& settings);
  ~Client();

  /// Get an administrative interface for the broker.
  AdminChannel GetAdminChannel();

  /// Get a publisher interface for the broker.
  Channel GetChannel();

  /// Get a reliable publisher interface for the broker (publisher-confirms)
  ReliableChannel GetReliableChannel();

 protected:
  Client(clients::dns::Resolver& resolver, const ClientSettings& settings);

 private:
  friend class ConsumerBase;
  utils::FastPimpl<ClientImpl, 232, 8> impl_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
