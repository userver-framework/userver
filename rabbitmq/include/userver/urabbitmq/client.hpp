#pragma once

#include <memory>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/utils/fast_pimpl.hpp>

#include <userver/urabbitmq/client_settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class AdminChannel;
class Channel;
class ConsumerBase;
class ClientImpl;

/// Interface for communicating with a cluster of RabbitMQ servers.
class Client final : public std::enable_shared_from_this<Client> {
 public:
  Client(clients::dns::Resolver& resolver, const ClientSettings& settings);
  ~Client();

  /// Get an administrative interface for the cluster.
  AdminChannel GetAdminChannel();

  /// Get a publisher interface for the cluster.
  Channel GetChannel();

 private:
  friend class ConsumerBase;

  utils::FastPimpl<ClientImpl, 72, 8> impl_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
