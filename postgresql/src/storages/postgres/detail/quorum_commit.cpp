#include "storages/postgres/detail/quorum_commit.hpp"

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include <storages/postgres/detail/connection.hpp>

#include <engine/deadline.hpp>
#include <engine/task/task_with_result.hpp>
#include <logging/log.hpp>
#include <rcu/rcu.hpp>
#include <utils/assert.hpp>
#include <utils/periodic_task.hpp>
#include <utils/scope_guard.hpp>

namespace storages::postgres::detail {

namespace {

constexpr auto kCheckTimeout = std::chrono::seconds{1};
constexpr auto kDiscoveryInterval = std::chrono::seconds{1};

using Rtt = std::chrono::microseconds;
constexpr Rtt kUnknownRtt{-1};

// Special connection ID to ease detection in logs
constexpr uint32_t kConnectionId = 4'100'200'300;
constexpr const char* kDiscoveryTaskName = "pg_topology";

const std::string kSelectSyncSlaveNames =
    R"~(select distinct application_name from pg_stat_replication where sync_state = 'sync')~";

struct HostState {
  explicit HostState(const Dsn& dsn)
      : app_name{EscapeHostName(OptionsFromDsn(dsn).host)} {}

  ~HostState() {
    // close connections synchronously
    if (connection) connection->Close();
  }

  std::unique_ptr<Connection> connection;

  const std::string host_port;
  // In pg_stat_replication slaves' host names are escaped and the column is
  // called `application_name`
  std::string app_name;

  ClusterHostType role = ClusterHostType::kUnknown;
  Rtt roundtrip_time{kUnknownRtt};
  std::vector<std::string> detected_sync_slaves;
};

}  // namespace

class QuorumCommitTopology::Impl {
 public:
  Impl(engine::TaskProcessor& bg_task_processor, DsnList dsns,
       const ConnectionSettings& conn_settings,
       const CommandControl& default_cmd_ctl,
       const testsuite::PostgresControl& testsuite_pg_ctl,
       error_injection::Settings ei_settings);
  ~Impl();

  const DsnList& GetDsnList() const;
  rcu::ReadablePtr<DsnIndicesByType> GetDsnIndicesByType() const;
  rcu::ReadablePtr<DsnIndices> GetDsnIndicesByRoundtripTime() const;

  void RunDiscovery();

  void StartPeriodicTask();
  void StopPeriodicTask();

 private:
  void RunCheck(DsnIndex);

  /// Background task processor passed to connection objects
  engine::TaskProcessor& bg_task_processor_;
  /// All DSNs handled by this topology discovery component
  DsnList dsns_;
  /// Individual connection settings
  ConnectionSettings conn_settings_;
  CommandControl default_cmd_ctl_;
  testsuite::PostgresControl testsuite_pg_ctl_;
  const error_injection::Settings ei_settings_;

  /// Host states array
  std::vector<HostState> host_states_;

  /// Currently determined host types exposed to the client
  rcu::Variable<DsnIndicesByType> dsn_indices_by_type_;

  /// Currently accessible hosts by roundtrip time
  rcu::Variable<DsnIndices> dsn_indices_by_rtt_;

  ::utils::PeriodicTask discovery_task_;
};

QuorumCommitTopology::Impl::Impl(
    engine::TaskProcessor& bg_task_processor, DsnList dsns,
    const ConnectionSettings& conn_settings,
    const CommandControl& default_cmd_ctl,
    const testsuite::PostgresControl& testsuite_pg_ctl,
    error_injection::Settings ei_settings)
    : bg_task_processor_{bg_task_processor},
      dsns_{std::move(dsns)},
      conn_settings_{conn_settings},
      default_cmd_ctl_{default_cmd_ctl},
      testsuite_pg_ctl_{testsuite_pg_ctl},
      ei_settings_{std::move(ei_settings)},
      host_states_{dsns_.begin(), dsns_.end()} {
  RunDiscovery();
  StartPeriodicTask();
}

QuorumCommitTopology::Impl::~Impl() { StopPeriodicTask(); }

const DsnList& QuorumCommitTopology::Impl::GetDsnList() const { return dsns_; }

rcu::ReadablePtr<QuorumCommitTopology::DsnIndicesByType>
QuorumCommitTopology::Impl::GetDsnIndicesByType() const {
  return dsn_indices_by_type_.Read();
}

rcu::ReadablePtr<QuorumCommitTopology::DsnIndices>
QuorumCommitTopology::Impl::GetDsnIndicesByRoundtripTime() const {
  return dsn_indices_by_rtt_.Read();
}

void QuorumCommitTopology::Impl::RunDiscovery() {
  std::vector<engine::TaskWithResult<void>> tasks;
  tasks.reserve(dsns_.size());
  for (DsnIndex i = 0; i < dsns_.size(); ++i) {
    tasks.emplace_back(engine::impl::Async([this, i] { RunCheck(i); }));
  }
  UASSERT(tasks.size() == dsns_.size());
  DsnIndices alive_dsn_indices;
  for (DsnIndex i = 0; i < tasks.size(); ++i) {
    tasks[i].Get();
    const auto& status = host_states_[i];
    LOG_DEBUG() << status.app_name << " is " << status.role << " rtt "
                << status.roundtrip_time.count() << "us";
    if (status.role != ClusterHostType::kUnknown) {
      alive_dsn_indices.push_back(i);
    }
  }
  // At this stage alive indices can point only to two types of hosts -
  // master and slave. The record for master can contain names of sync slaves
  auto master = std::find_if(
      host_states_.begin(), host_states_.end(),
      [](const auto& s) { return s.role == ClusterHostType::kMaster; });

  // O(N^2), seems OK for expected number of items
  if (master != host_states_.end() && !master->detected_sync_slaves.empty()) {
    for (const auto& ss_app_name : master->detected_sync_slaves) {
      for (DsnIndex idx : alive_dsn_indices) {
        auto& state = host_states_[idx];
        if (state.app_name == ss_app_name) {
          LOG_DEBUG() << state.app_name << " is a sync slave";
          state.role = ClusterHostType::kSyncSlave;
        }
      }
    }
  }

  std::sort(alive_dsn_indices.begin(), alive_dsn_indices.end(),
            [this](DsnIndex lhs, DsnIndex rhs) {
              return host_states_[lhs].roundtrip_time <
                     host_states_[rhs].roundtrip_time;
            });
  DsnIndicesByType dsn_indices_by_type;
  for (DsnIndex idx : alive_dsn_indices) {
    const auto& state = host_states_[idx];

    dsn_indices_by_type[state.role].push_back(idx);
    // Always allow using sync slaves for slave requests, mainly for transition
    // purposes -- TAXICOMMON-2006
    if (state.role == ClusterHostType::kSyncSlave) {
      dsn_indices_by_type[ClusterHostType::kSlave].push_back(idx);
    }
  }
  dsn_indices_by_type_.Assign(std::move(dsn_indices_by_type));
  dsn_indices_by_rtt_.Assign(alive_dsn_indices);
}

void QuorumCommitTopology::Impl::StartPeriodicTask() {
  using Flags = ::utils::PeriodicTask::Flags;

  discovery_task_.Start(kDiscoveryTaskName,
                        {kDiscoveryInterval, {Flags::kStrong}},
                        [this] { RunDiscovery(); });
}

void QuorumCommitTopology::Impl::StopPeriodicTask() { discovery_task_.Stop(); }

void QuorumCommitTopology::Impl::RunCheck(DsnIndex idx) {
  UASSERT(idx < dsns_.size());
  const auto& dsn = dsns_[idx];
  auto& state = host_states_[idx];

  ::utils::ScopeGuard role_check_guard([&state] {
    state.connection.reset();
    state.role = ClusterHostType::kUnknown;
    state.roundtrip_time = kUnknownRtt;
    state.detected_sync_slaves.clear();
  });

  if (!state.connection) {
    try {
      state.connection = Connection::Connect(
          dsn, bg_task_processor_, kConnectionId, conn_settings_,
          default_cmd_ctl_, testsuite_pg_ctl_, ei_settings_);
    } catch (const ConnectionError& e) {
      LOG_ERROR() << "Failed to connect to " << DsnCutPassword(dsn) << ": "
                  << e;
      return;
    }
  }
  auto deadline = testsuite_pg_ctl_.MakeExecuteDeadline(kCheckTimeout);
  auto start = std::chrono::steady_clock::now();
  try {
    auto ro = state.connection->CheckReadOnly(deadline);
    state.role = ro ? ClusterHostType::kSlave : ClusterHostType::kMaster;
    state.roundtrip_time = std::chrono::duration_cast<Rtt>(
        std::chrono::steady_clock::now() - start);
    if (state.role == ClusterHostType::kMaster) {
      auto res = state.connection->Execute(kSelectSyncSlaveNames);
      LOG_DEBUG() << res.Size() << " sync slaves detected";
      state.detected_sync_slaves = res.AsContainer<std::vector<std::string>>();
    }
    role_check_guard.Release();
  } catch (const ConnectionError& e) {
    LOG_ERROR() << "Broken connection with " << DsnCutPassword(dsn) << ": "
                << e;
  }
}

QuorumCommitTopology::QuorumCommitTopology(
    engine::TaskProcessor& bg_task_processor, DsnList dsns,
    const ConnectionSettings& conn_settings,
    const CommandControl& default_cmd_ctl,
    const testsuite::PostgresControl& testsuite_pg_ctl,
    error_injection::Settings ei_settings)
    : pimpl_(bg_task_processor, std::move(dsns), conn_settings, default_cmd_ctl,
             testsuite_pg_ctl, std::move(ei_settings)){};

QuorumCommitTopology::~QuorumCommitTopology() = default;

const DsnList& QuorumCommitTopology::GetDsnList() const {
  return pimpl_->GetDsnList();
}

rcu::ReadablePtr<QuorumCommitTopology::DsnIndicesByType>
QuorumCommitTopology::GetDsnIndicesByType() const {
  return pimpl_->GetDsnIndicesByType();
}

rcu::ReadablePtr<QuorumCommitTopology::DsnIndices>
QuorumCommitTopology::GetDsnIndicesByRoundtripTime() const {
  return pimpl_->GetDsnIndicesByRoundtripTime();
}

}  // namespace storages::postgres::detail
