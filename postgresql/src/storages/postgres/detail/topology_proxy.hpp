#pragma once

#include <storages/postgres/detail/topology.hpp>

namespace storages {
namespace postgres {
namespace detail {

class ClusterTopologyProxy : public ClusterTopology {
 public:
  explicit ClusterTopologyProxy(const ClusterDescription& cluster_desc);

  const DSNList& GetDsnList() const;
  HostsByType GetHostsByType() const override;

 private:
  void AddHost(ClusterHostType host_type, const std::string& host);

 private:
  HostsByType hosts_by_type;
  DSNList dsn_list_;
};

}  // namespace detail
}  // namespace postgres
}  // namespace storages
