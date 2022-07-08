#pragma once

#include <memory>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class AdminChannel;
class Channel;
class ConsumerBase;
class ClusterImpl;

/// Interface for communicating with a cluster of RabbitMQ servers.
class Cluster final : public std::enable_shared_from_this<Cluster> {
 public:
  Cluster(clients::dns::Resolver& resolver);
  ~Cluster();

  /// Get an administrative interface for the cluster.
  AdminChannel GetAdminChannel();

  /// Get a publisher interface for the cluster.
  Channel GetChannel();

 private:
  friend class ConsumerBase;

  utils::FastPimpl<ClusterImpl, 1136, 8> impl_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
