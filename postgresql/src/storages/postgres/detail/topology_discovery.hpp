#pragma once

#include <storages/postgres/detail/topology.hpp>

#include <boost/variant.hpp>

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

  void BuildEscapedNames();
  void CreateConnections(const DSNList& dsn_list);
  void StartPeriodicUpdates();
  void StopRunningTasks();
  [[nodiscard]] engine::TaskWithResult<ConnectionPtr> Connect(std::string dsn);
  void Reconnect(size_t index);
  void CloseConnection(ConnectionPtr conn_ptr);

  Connection* GetConnectionOrThrow(size_t index) const noexcept(false);
  Connection* GetConnectionOrNull(size_t index);

  void CheckTopology();
  size_t CheckHostsAndFindMaster(HostTypeList& host_types);
  void FindSyncSlaves(HostTypeList& host_types, size_t master_index);
  std::string DumpTopologyState(const HostTypeList& host_types) const;

 private:
  using ConnectionTask = engine::TaskWithResult<ConnectionPtr>;

  struct ConnectionState {
    std::string dsn;
    boost::variant<ConnectionPtr, ConnectionTask> conn_variant;
    size_t failed_reconnects = 0;
  };

  engine::TaskProcessor& bg_task_processor_;
  HostTypeList host_types_;
  std::vector<ConnectionState> connections_;
  std::unordered_map<std::string, size_t> escaped_to_dsn_index_;
  ::utils::PeriodicTask periodic_task_;
  engine::Mutex update_mutex_;
  mutable engine::Mutex hosts_mutex_;
};

}  // namespace detail
}  // namespace postgres
}  // namespace storages
