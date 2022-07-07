#include <userver/urabbitmq/cluster.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

std::shared_ptr<AdminChannel> Cluster::GetAdminChannel() { return nullptr; }

std::shared_ptr<Channel> Cluster::GetChannel() { return nullptr; }

}  // namespace urabbitmq

USERVER_NAMESPACE_END
