#pragma once

#include <storages/postgres/detail/topology.hpp>

#include <boost/variant.hpp>

#include <engine/mutex.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/task/task_with_result.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <utils/statistics/relaxed_counter.hpp>

namespace storages {
namespace postgres {
namespace detail {

class ClusterTopologyDiscovery : public ClusterTopology {
 public:
  ClusterTopologyDiscovery(engine::TaskProcessor& bg_task_processor,
                           const DSNList& dsn_list);
  ~ClusterTopologyDiscovery();

  HostsByType GetHostsByType() const override;
  void CheckTopology() override;
  void OperationFailed(const std::string& dsn) override;

 private:
  void BuildIndexes();
  void CreateConnections(const DSNList& dsn_list);
  void StopRunningTasks();
  [[nodiscard]] engine::TaskWithResult<ConnectionPtr> Connect(std::string dsn);
  void Reconnect(size_t index);
  void CloseConnection(ConnectionPtr conn_ptr);

  Connection* GetConnectionOrThrow(size_t index) const noexcept(false);
  Connection* GetConnectionOrNull(size_t index);

  size_t CheckHostsAndFindMaster();
  void FindSyncSlaves(size_t master_index);
  std::string DumpTopologyState() const;
  void UpdateHostsByType();

 private:
  using ConnectionTask = engine::TaskWithResult<ConnectionPtr>;

  template <typename T>
  class RelaxedAtomic : public ::utils::statistics::RelaxedCounter<T> {
   public:
    using ::utils::statistics::RelaxedCounter<T>::RelaxedCounter;
    RelaxedAtomic(RelaxedAtomic&& other) noexcept
        : ::utils::statistics::RelaxedCounter<T>(other.Load()) {}
  };

  struct ConnectionState {
    ConnectionState(const std::string& dsn, ConnectionTask&& task);
    ConnectionState(ConnectionState&&) noexcept = default;
    ConnectionState& operator=(ConnectionState&&) = default;

    std::string dsn;
    boost::variant<ConnectionPtr, ConnectionTask> conn_variant;
    ClusterHostType host_type;
    size_t failed_reconnects;

    // The data below is modified concurrently

    // We don't care about exact accuracy
    RelaxedAtomic<size_t> failed_operations;
  };

  engine::TaskProcessor& bg_task_processor_;
  std::vector<ConnectionState> connections_;
  mutable engine::Mutex hosts_mutex_;
  HostsByType hosts_by_type_;
  std::unordered_map<std::string, size_t> dsn_to_index_;
  std::unordered_map<std::string, size_t> escaped_to_dsn_index_;
  std::atomic_flag update_lock_;
};

}  // namespace detail
}  // namespace postgres
}  // namespace storages
