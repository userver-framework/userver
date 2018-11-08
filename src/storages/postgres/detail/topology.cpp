#include <storages/postgres/detail/topology.hpp>

#include <logging/log.hpp>
#include <storages/postgres/exceptions.hpp>

namespace storages {
namespace postgres {
namespace detail {

ClusterTopology::ClusterTopology(const ClusterDescription& cluster_desc) {
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

const DSNList& ClusterTopology::GetDsnList() const { return dsn_list_; }

ClusterTopology::HostIndices ClusterTopology::GetHostsByType(
    ClusterHostType ht) const {
  auto it = host_to_indices_.find(ht);
  if (it == host_to_indices_.end()) {
    return {};
  }
  return it->second;
}

void ClusterTopology::AddHost(ClusterHostType host_type,
                              const std::string& host) {
  if (host.empty()) {
    LOG_INFO() << "No " << host_type << " host added";
    return;
  }

  const auto insert_ind = dsn_list_.size();
  dsn_list_.push_back(host);
  host_to_indices_[host_type].push_back(insert_ind);
  LOG_INFO() << "Added " << host_type << " host";
}

}  // namespace detail
}  // namespace postgres
}  // namespace storages
