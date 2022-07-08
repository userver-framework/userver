#include <userver/urabbitmq/cluster.hpp>

#include <userver/urabbitmq/admin_channel.hpp>
#include <userver/urabbitmq/channel.hpp>

#include <urabbitmq/channel_pool.hpp>
#include <urabbitmq/cluster_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

Cluster::Cluster(clients::dns::Resolver& resolver)
    : impl_{std::make_unique<ClusterImpl>(resolver)} {}

Cluster::~Cluster() = default;

AdminChannel Cluster::GetAdminChannel() {
  return AdminChannel{shared_from_this(), impl_->GetUnreliable()};
}

Channel Cluster::GetChannel() {
  return Channel{shared_from_this(), impl_->GetUnreliable(),
                 impl_->GetReliable()};
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
