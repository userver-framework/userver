#pragma once

#include <memory>

#include <userver/clients/dns/resolver_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class AdminChannel;
class Channel;
class ConsumerBase;
class ClusterImpl;

class Cluster final : public std::enable_shared_from_this<Cluster> {
 public:
  Cluster(clients::dns::Resolver& resolver);
  ~Cluster();

  AdminChannel GetAdminChannel();
  Channel GetChannel();

 private:
  friend class ConsumerBase;

  std::unique_ptr<ClusterImpl> impl_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
