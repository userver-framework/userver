#include <storages/postgres/detail/topology_proxy.hpp>

#include <logging/log.hpp>
#include <storages/postgres/exceptions.hpp>

namespace storages {
namespace postgres {
namespace detail {

ClusterTopologyProxy::ClusterTopologyProxy(
    const ClusterDescription& cluster_desc) {
  if (cluster_desc.master_dsn_.empty()) {
    throw ClusterUnavailable("Master host is unavailable");
  }

  dsn_list_.reserve(cluster_desc.slave_dsns_.size() +
                    2 /*master + sync-slave*/);
  AddHost(ClusterHostType::kMaster, cluster_desc.master_dsn_);
  AddHost(ClusterHostType::kSyncSlave, cluster_desc.sync_slave_dsn_);

  for (auto&& slave_host : cluster_desc.slave_dsns_) {
    AddHost(ClusterHostType::kSlave, slave_host);
  }
}

const DSNList& ClusterTopologyProxy::GetDsnList() const { return dsn_list_; }

ClusterTopology::HostsByType ClusterTopologyProxy::GetHostsByType() const {
  return hosts_by_type;
}

void ClusterTopologyProxy::AddHost(ClusterHostType host_type,
                                   const std::string& host) {
  if (host.empty()) {
    LOG_INFO() << "No " << host_type << " host added";
    return;
  }

  dsn_list_.push_back(host);
  hosts_by_type[host_type].push_back(host);
  LOG_INFO() << "Added " << host_type << " host";
}

}  // namespace detail
}  // namespace postgres
}  // namespace storages
