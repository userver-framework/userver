#pragma once

#include <unordered_map>

#include <storages/postgres/detail/topology.hpp>

#include <engine/mutex.hpp>
#include <engine/task/task_processor.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <utils/periodic_task.hpp>

namespace storages {
namespace postgres {
namespace detail {

class ClusterTopologyDiscovery : public ClusterTopology {
 public:
  ClusterTopologyDiscovery(engine::TaskProcessor& bg_task_processor,
                           const DSNList& dsn_list);
  ~ClusterTopologyDiscovery();

  HostsByType GetHostsByType() const override;

 private:
  using HostTypeList = std::vector<ClusterHostType>;

  void InitHostInfo(const DSNList& dsn_list);
  void CreateConnections();
  void StartPeriodicUpdates();
  [[nodiscard]] engine::TaskWithResult<ConnectionPtr> Connect(std::string dsn);

  void CheckTopology();
  size_t FindMaster(HostTypeList& host_types) const;
  void FindSyncSlaves(HostTypeList& host_types, size_t master_index) const;

 private:
  engine::TaskProcessor& bg_task_processor_;
  std::vector<ConnectionPtr> connections_;
  HostTypeList host_types_;
  std::unordered_map<std::string, size_t> escaped_to_dsn_index_;
  ::utils::PeriodicTask periodic_task_;
  engine::Mutex update_mutex_;
  mutable engine::Mutex hosts_mutex_;
};

}  // namespace detail
}  // namespace postgres
}  // namespace storages
