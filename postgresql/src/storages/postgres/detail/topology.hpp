#pragma once

#include <chrono>

#include <boost/variant.hpp>

#include <engine/mutex.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/task/task_with_result.hpp>

#include <storages/postgres/cluster_types.hpp>
#include <storages/postgres/detail/connection.hpp>

#include <utils/statistics/relaxed_counter.hpp>
#include <utils/swappingsmart.hpp>

namespace storages {
namespace postgres {
namespace detail {

struct ClusterHostTypeHash {
  size_t operator()(const ClusterHostType& ht) const noexcept {
    return static_cast<size_t>(ht);
  }
};

class ClusterTopology {
 public:
  using HostsByType =
      std::unordered_map<ClusterHostType, DSNList, ClusterHostTypeHash>;

  enum class HostAvailability {
    kOffline,    /// The host went offline
    kPreOnline,  /// Next iteration the host might go online
    kOnline,     /// The host is back online
  };

  using HostAvailabilityChanges =
      std::unordered_map<std::string, HostAvailability>;

 public:
  ClusterTopology(engine::TaskProcessor& bg_task_processor,
                  const ClusterDescription& desc);

  ~ClusterTopology();

  HostsByType GetHostsByType() const;
  HostAvailabilityChanges CheckTopology();
  void OperationFailed(const std::string& dsn);

  DSNList const& GetDsnList() const { return dsns_; }

  // TODO Move constants to config
  // Topology check interval. Should be no less than kMinCheckDuration
  static const std::chrono::seconds kUpdateInterval;

 private:
  class ClusterDescriptionVisitor;
  friend class ClusterDescriptionVisitor;
  /// State of host checks in progress
  struct HostChecks {
    enum class Stage {
      /// Availability (and master) check stage
      kAvailability,
      /// Sync slaves check stage
      kSyncSlaves,
    };

    /// Host state check operation in progress (if any)
    std::unique_ptr<engine::Task> task;
    /// Current check stage
    Stage stage;
  };

  using ChecksList = std::vector<HostChecks>;
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
  engine::Task* CheckAvailability(size_t index, ChecksList& checks);
  engine::Task* DetectMaster(size_t index, ChecksList& checks);
  engine::Task* CheckSyncSlaves(size_t master_index, ChecksList& checks,
                                Connection* conn);
  engine::Task* DetectSyncSlaves(size_t master_index, ChecksList& checks);
  template <typename T>
  engine::Task* SetCheckStage(size_t index, ChecksList& checks, T&& task,
                              HostChecks::Stage stage) const;
  HostAvailabilityChanges UpdateHostTypes();
  std::string DumpTopologyState() const;
  HostsByType BuildHostsByType() const;

 private:
  /// Keeps changes happening with the host
  struct StateChages {
    /// Host type determined after last check
    ClusterHostType last_check_type;
    /// Host type kept as potential new type after number of checks
    ClusterHostType new_type;
    /// Number of times new host type seen in a row
    size_t count;
  };

  /// Host state info
  struct HostState {
    HostState(const std::string& dsn, ConnectionTask&& task);
    HostState(HostState&&) noexcept = default;
    HostState& operator=(HostState&&) = default;

    /// Host DSN
    std::string dsn;
    /// Either a working connection to the host or a task if connection is being
    /// established
    boost::variant<std::unique_ptr<Connection>, ConnectionTask> conn_variant;
    /// Currently exposed host type (role)
    ClusterHostType host_type;
    /// Number of failed reconnects in a row
    size_t failed_reconnects;

    /// Current state of changes to the host
    StateChages changes;

    // The data below is modified concurrently

    /// Failed operations counter helping immediately mark the host as offline
    /// when the given threshold is reached
    /// @note We don't care about exact accuracy so it's a relaxed atomic
    ::utils::statistics::RelaxedCounter<size_t> failed_operations;
  };

  /// Background task processor passed to connection objects
  engine::TaskProcessor& bg_task_processor_;
  /// Duration of topology check routine
  std::chrono::milliseconds check_duration_;
  /// Host states array
  std::vector<HostState> host_states_;
  /// Currently determined host types exposed to the client
  ::utils::SwappingSmart<HostsByType> hosts_by_type_;
  /// All DSNs handled by this topology discovery component
  DSNList dsns_;
  /// Host DSN names to array index mapping
  std::unordered_map<std::string, size_t> dsn_to_index_;
  /// Index to search for sync slave names returned from the specific call to Pg
  std::unordered_map<std::string, size_t> escaped_to_dsn_index_;
  /// Initial check flag
  bool initial_check_;
};

using ClusterTopologyPtr = std::unique_ptr<ClusterTopology>;

}  // namespace detail
}  // namespace postgres
}  // namespace storages
