#pragma once

/// @file userver/urabbitmq/client.hpp
/// @brief @copybrief urabbitmq::Client

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

/// @ingroup userver_clients
///
/// @brief Interface for communicating with a RabbitMQ cluster.
///
/// Usually retrieved from components::RabbitMQ component.
class Client : public std::enable_shared_from_this<Client> {
 public:
  /// Client factory function
  /// @param resolver asynchronous DNS resolver
  /// @param settings client settings
  static std::shared_ptr<Client> Create(clients::dns::Resolver& resolver,
                                        const ClientSettings& settings);
  /// Client destructor
  ~Client();

  /// @brief Get an administrative interface for the broker.
  AdminChannel GetAdminChannel();

  /// @brief Get a publisher interface for the broker.
  Channel GetChannel();

  /// @brief Get a reliable publisher interface for the broker
  /// (publisher-confirms)
  ReliableChannel GetReliableChannel();

 protected:
  Client(clients::dns::Resolver& resolver, const ClientSettings& settings);

 private:
  friend class ConsumerBase;
  utils::FastPimpl<ClientImpl, 232, 8> impl_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
