#pragma once

#include <storages/postgres/detail/topology.hpp>

#include <chrono>

#include <boost/variant.hpp>

#include <engine/mutex.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/task/task_with_result.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <utils/statistics/relaxed_counter.hpp>
#include <utils/swappingsmart.hpp>

namespace storages {
namespace postgres {
namespace detail {

class ClusterTopologyDiscovery : public ClusterTopology {
 public:
  ClusterTopologyDiscovery(engine::TaskProcessor& bg_task_processor,
                           const DSNList& dsn_list);
  ~ClusterTopologyDiscovery();

  HostsByType GetHostsByType() const override;
  HostAvailabilityChanges CheckTopology() override;
  void OperationFailed(const std::string& dsn) override;

  // TODO Move constants to config
  // Topology check interval. Should be no less than kMinCheckDuration
  static const std::chrono::seconds kUpdateInterval;

 private:
  using ConnectionTask = engine::TaskWithResult<std::unique_ptr<Connection>>;

  void BuildIndexes();
  void CreateConnections(const DSNList& dsn_list);
  void StopRunningTasks();
  [[nodiscard]] ConnectionTask Connect(std::string dsn);
  void Reconnect(size_t index);
  void CloseConnection(std::unique_ptr<Connection> conn_ptr);
  void AccountHostTypeChange(size_t index);
  bool ShouldChangeHostType(size_t index) const;

  Connection* GetConnectionOrThrow(size_t index) const noexcept(false);
  Connection* GetConnectionOrNull(size_t index);

  void CheckHosts(const std::chrono::steady_clock::time_point& check_end_point);
  engine::Task* CheckAvailability(size_t index);
  engine::Task* CheckIfMaster(size_t index,
                              engine::TaskWithResult<ClusterHostType>& task);
  engine::Task* FindSyncSlaves(size_t master_index, Connection* conn);
  engine::Task* CheckSyncSlaves(
      size_t master_index, engine::TaskWithResult<std::vector<size_t>>& task);
  HostAvailabilityChanges UpdateHostTypes();
  std::string DumpTopologyState() const;
  HostsByType BuildHostsByType() const;

 private:
  template <typename T>
  class RelaxedAtomic : public ::utils::statistics::RelaxedCounter<T> {
   public:
    using ::utils::statistics::RelaxedCounter<T>::RelaxedCounter;
    RelaxedAtomic(RelaxedAtomic&& other) noexcept
        : ::utils::statistics::RelaxedCounter<T>(other.Load()) {}
  };

  struct HostChecks {
    enum class Stage {
      kReconnect,
      kAvailability,
      kSyncSlaves,
    };

    std::unique_ptr<engine::Task> task;
    Stage stage;
  };

  struct StateChages {
    ClusterHostType last_check_type;
    ClusterHostType new_type;
    size_t count;
  };

  struct HostState {
    HostState(const std::string& dsn, ConnectionTask&& task);
    HostState(HostState&&) noexcept = default;
    HostState& operator=(HostState&&) = default;

    std::string dsn;
    boost::variant<std::unique_ptr<Connection>, ConnectionTask> conn_variant;
    ClusterHostType host_type;
    size_t failed_reconnects;

    StateChages changes;

    HostChecks checks;

    // The data below is modified concurrently

    // We don't care about exact accuracy
    RelaxedAtomic<size_t> failed_operations;
  };

  engine::TaskProcessor& bg_task_processor_;
  std::chrono::milliseconds check_duration_;
  std::vector<HostState> host_states_;
  ::utils::SwappingSmart<HostsByType> hosts_by_type_;
  std::unordered_map<std::string, size_t> dsn_to_index_;
  std::unordered_map<std::string, size_t> escaped_to_dsn_index_;
  bool initial_check_;
};

}  // namespace detail
}  // namespace postgres
}  // namespace storages
