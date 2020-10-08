#include "storages/postgres/detail/quorum_commit.hpp"

#include <algorithm>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/internal_pg_types.hpp>
#include <storages/postgres/io/pg_type_parsers.hpp>

#include <crypto/openssl.hpp>
#include <engine/deadline.hpp>
#include <engine/task/task_with_result.hpp>
#include <logging/log.hpp>
#include <rcu/rcu.hpp>
#include <utils/assert.hpp>
#include <utils/periodic_task.hpp>
#include <utils/scope_guard.hpp>
#include <utils/str_icase.hpp>
#include <utils/strong_typedef.hpp>

namespace storages::postgres::detail {

namespace {

constexpr auto kCheckTimeout = std::chrono::seconds{1};
constexpr auto kDiscoveryInterval = std::chrono::seconds{1};

using Rtt = std::chrono::microseconds;
constexpr Rtt kUnknownRtt{-1};

using ReplicationLag = std::chrono::milliseconds;

// Special connection ID to ease detection in logs
constexpr uint32_t kConnectionId = 4'100'200'300;
constexpr const char* kDiscoveryTaskName = "pg_topology";

const std::string kShowSyncStandbyNames = "SHOW synchronous_standby_names";

// Master doesn't provide last xact timestamp without `track_commit_timestamp`
// enabled, use current server time as an approximation.
// Also note that all times here are sourced from the master.
const std::string kGetMasterWalInfo = "SELECT pg_current_wal_lsn(), now()";
const std::string kGetSlaveWalInfo =
    "SELECT pg_last_wal_replay_lsn(), pg_last_xact_replay_timestamp()";

struct HostState {
  explicit HostState(const Dsn& dsn)
      : app_name{EscapeHostName(OptionsFromDsn(dsn).host)} {}

  ~HostState() {
    // close connections synchronously
    if (connection) connection->Close();
  }

  void Reset() noexcept {
    connection.reset();
    role = ClusterHostType::kNone;
    is_readonly = true;
    roundtrip_time = kUnknownRtt;
    wal_lsn = kUnknownLsn;
    current_xact_timestamp = {};
    detected_sync_slaves.clear();
  }

  std::unique_ptr<Connection> connection;

  const std::string host_port;
  // In pg_stat_replication slaves' host names are escaped and the column is
  // called `application_name`
  std::string app_name;

  ClusterHostType role = ClusterHostType::kNone;
  bool is_readonly = true;
  Rtt roundtrip_time{kUnknownRtt};
  Lsn wal_lsn{kUnknownLsn};
  std::chrono::system_clock::time_point current_xact_timestamp;
  std::vector<std::string> detected_sync_slaves;
};

std::string_view ConsumeToken(std::string_view& sv) {
  static constexpr auto kSep = " ,()\"";

  const auto sep_end = sv.find_first_not_of(kSep);
  if (sep_end == std::string_view::npos) return {};
  sv.remove_prefix(sep_end);

  const auto tok_end = sv.find_first_of(kSep);
  if (tok_end == std::string::npos) return std::exchange(sv, {});

  auto token = sv.substr(0, tok_end);
  sv.remove_prefix(tok_end);
  return token;
}

size_t ParseSize(std::string_view token) {
  size_t result = 0;
  for (const char c : token) {
    if (c < '0' || c > '9') break;
    result *= 10;
    result += c - '0';
  }
  return result;
}

}  // namespace

class QuorumCommitTopology::Impl {
 public:
  Impl(engine::TaskProcessor& bg_task_processor, DsnList dsns,
       const TopologySettings& topology_settings,
       const ConnectionSettings& conn_settings,
       const CommandControl& default_cmd_ctl,
       const testsuite::PostgresControl& testsuite_pg_ctl,
       error_injection::Settings ei_settings);
  ~Impl();

  const DsnList& GetDsnList() const;
  rcu::ReadablePtr<DsnIndicesByType> GetDsnIndicesByType() const;
  rcu::ReadablePtr<DsnIndices> GetAliveDsnIndices() const;
  const std::vector<decltype(InstanceStatistics::topology)>& GetDsnStatistics()
      const;

  void RunDiscovery();

  void StartPeriodicTask();
  void StopPeriodicTask();

 private:
  void RunCheck(DsnIndex);

  /// Background task processor passed to connection objects
  engine::TaskProcessor& bg_task_processor_;
  /// All DSNs handled by this topology discovery component
  DsnList dsns_;
  const TopologySettings topology_settings_;
  ConnectionSettings conn_settings_;
  CommandControl default_cmd_ctl_;
  testsuite::PostgresControl testsuite_pg_ctl_;
  const error_injection::Settings ei_settings_;

  /// Host states array
  std::vector<HostState> host_states_;

  /// Currently determined host types exposed to the client, ordered by rtt
  rcu::Variable<DsnIndicesByType> dsn_indices_by_type_;

  /// Currently accessible hosts
  rcu::Variable<DsnIndices> alive_dsn_indices_;

  /// Public availability info
  std::vector<decltype(InstanceStatistics::topology)> dsn_stats_;

  ::utils::PeriodicTask discovery_task_;
};

QuorumCommitTopology::Impl::Impl(
    engine::TaskProcessor& bg_task_processor, DsnList dsns,
    const TopologySettings& topology_settings,
    const ConnectionSettings& conn_settings,
    const CommandControl& default_cmd_ctl,
    const testsuite::PostgresControl& testsuite_pg_ctl,
    error_injection::Settings ei_settings)
    : bg_task_processor_{bg_task_processor},
      dsns_{std::move(dsns)},
      topology_settings_{topology_settings},
      conn_settings_{conn_settings},
      default_cmd_ctl_{default_cmd_ctl},
      testsuite_pg_ctl_{testsuite_pg_ctl},
      ei_settings_{std::move(ei_settings)},
      host_states_{dsns_.begin(), dsns_.end()},
      dsn_stats_(dsns_.size()) {
  crypto::impl::Openssl::Init();
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
QuorumCommitTopology::Impl::GetAliveDsnIndices() const {
  return alive_dsn_indices_.Read();
}

const std::vector<decltype(InstanceStatistics::topology)>&
QuorumCommitTopology::Impl::GetDsnStatistics() const {
  return dsn_stats_;
}

void QuorumCommitTopology::Impl::RunDiscovery() {
  std::vector<engine::TaskWithResult<void>> tasks;
  tasks.reserve(dsns_.size());
  for (DsnIndex i = 0; i < dsns_.size(); ++i) {
    tasks.emplace_back(engine::impl::Async([this, i] { RunCheck(i); }));
  }
  for (auto& task : tasks) task.Get();

  // Report states and find the master
  HostState* master = nullptr;
  std::chrono::system_clock::time_point max_slave_xact_timestamp;
  for (DsnIndex i = 0; i < host_states_.size(); ++i) {
    auto& state = host_states_[i];
    LOG_DEBUG() << state.app_name << " is " << state.role << ": rtt "
                << state.roundtrip_time.count() << "us, LSN " << state.wal_lsn
                << ", last xact time " << state.current_xact_timestamp;
    if (state.roundtrip_time != kUnknownRtt) {
      dsn_stats_[i].roundtrip_time.GetCurrentCounter().Account(
          std::chrono::duration_cast<std::chrono::milliseconds>(
              state.roundtrip_time)
              .count());
    }
    if (state.role == ClusterHostType::kMaster) {
      master = &state;
    } else if (state.role == ClusterHostType::kSlave) {
      max_slave_xact_timestamp =
          std::max(max_slave_xact_timestamp, state.current_xact_timestamp);
    }
  }

  // At this stage alive indices can point only to two types of hosts -
  // master and slaves. The record for master can contain names of sync
  // slaves.
  for (DsnIndex i = 0; i < host_states_.size(); ++i) {
    auto& slave = host_states_[i];
    if (slave.role != ClusterHostType::kSlave) continue;

    // xact timestamp can become stale when there are no writes.
    // - In normal case we compare against local slave time to avoid distributed
    // clock issues.
    // - In case slave replay LSN is not behind master's we can assume it's not
    // lagging.
    // - When the master is not available, no writes are possible and we compare
    // against the latest known xact time.
    const auto target_xact_timestamp =
        master
            ? (slave.wal_lsn < master->wal_lsn ? master->current_xact_timestamp
                                               : slave.current_xact_timestamp)
            : max_slave_xact_timestamp;
    // Target should be ahead of current, but clamp to zero for extra safety.
    auto slave_lag =
        std::max(ReplicationLag::zero(),
                 std::chrono::duration_cast<ReplicationLag>(
                     target_xact_timestamp - slave.current_xact_timestamp));

    dsn_stats_[i].replication_lag.GetCurrentCounter().Account(
        std::chrono::duration_cast<std::chrono::milliseconds>(slave_lag)
            .count());

    if (slave_lag > topology_settings_.max_replication_lag) {
      // Demote lagged slave
      LOG_INFO() << "Disabling slave " << slave.app_name
                 << " due to replication lag of " << slave_lag.count()
                 << " ms (max "
                 << topology_settings_.max_replication_lag.count() << " ms)";
      slave.role = ClusterHostType::kNone;
    } else if (master) {
      // Check for sync slave
      // O(N^2), seems OK for expected number of items
      for (const auto& ss_app_name : master->detected_sync_slaves) {
        if (::utils::StrIcaseEqual{}(slave.app_name, ss_app_name)) {
          LOG_DEBUG() << slave.app_name << " is a sync slave";
          slave.role = ClusterHostType::kSyncSlave;
        }
      }
    }
  }

  // Demote readonly master to slave
  if (master && master->is_readonly) {
    if (!testsuite_pg_ctl_.IsReadonlyMasterExpected()) {
      LOG_WARNING() << "Primary host is not writable, possibly due to "
                       "insufficient disk space";
    }
    master->role = ClusterHostType::kSlave;
  }

  DsnIndices alive_dsn_indices;
  for (DsnIndex i = 0; i < dsns_.size(); ++i) {
    if (host_states_[i].role != ClusterHostType::kNone) {
      alive_dsn_indices.push_back(i);
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
    // Always allow using sync slaves for slave requests, mainly for
    // transition purposes -- TAXICOMMON-2006
    if (state.role == ClusterHostType::kSyncSlave) {
      dsn_indices_by_type[ClusterHostType::kSlave].push_back(idx);
    }
  }
  dsn_indices_by_type_.Assign(std::move(dsn_indices_by_type));
  alive_dsn_indices_.Assign(std::move(alive_dsn_indices));
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

  ::utils::ScopeGuard role_check_guard([&state] { state.Reset(); });

  if (!state.connection) {
    try {
      state.connection = Connection::Connect(
          dsn, bg_task_processor_, kConnectionId, conn_settings_,
          default_cmd_ctl_, testsuite_pg_ctl_, ei_settings_);
    } catch (const ConnectionError& e) {
      LOG_WARNING() << "Failed to connect to " << DsnCutPassword(dsn) << ": "
                    << e;
      return;
    }
  }
  auto deadline = testsuite_pg_ctl_.MakeExecuteDeadline(kCheckTimeout);
  auto start = std::chrono::steady_clock::now();
  try {
    state.connection->RefreshReplicaState(deadline);
    state.role = state.connection->IsInRecovery() ? ClusterHostType::kSlave
                                                  : ClusterHostType::kMaster;
    state.is_readonly = state.connection->IsReadOnly();
    state.roundtrip_time = std::chrono::duration_cast<Rtt>(
        std::chrono::steady_clock::now() - start);

    std::optional<std::chrono::system_clock::time_point> current_xact_timestamp;
    state.connection
        ->Execute(state.connection->IsInRecovery() ? kGetSlaveWalInfo
                                                   : kGetMasterWalInfo)
        .Front()
        .To(state.wal_lsn, current_xact_timestamp);
    if (current_xact_timestamp) {
      state.current_xact_timestamp = *current_xact_timestamp;
    } else {
      LOG_INFO() << "Host " << DsnCutPassword(dsn) << " has no data, skipping";
      return;  // state will be reset
    }

    if (!state.connection->IsInRecovery()) {
      auto res = state.connection->Execute(kShowSyncStandbyNames);
      state.detected_sync_slaves =
          ParseSyncStandbyNames(res.AsSingleRow<std::string>());
      LOG_DEBUG() << state.detected_sync_slaves.size()
                  << " sync slaves detected";
    }
    role_check_guard.Release();
  } catch (const ConnectionError& e) {
    LOG_WARNING() << "Broken connection with " << DsnCutPassword(dsn) << ": "
                  << e;
  }
}

QuorumCommitTopology::QuorumCommitTopology(
    engine::TaskProcessor& bg_task_processor, DsnList dsns,
    const TopologySettings& topology_settings,
    const ConnectionSettings& conn_settings,
    const CommandControl& default_cmd_ctl,
    const testsuite::PostgresControl& testsuite_pg_ctl,
    error_injection::Settings ei_settings)
    : pimpl_(bg_task_processor, std::move(dsns), topology_settings,
             conn_settings, default_cmd_ctl, testsuite_pg_ctl,
             std::move(ei_settings)){};

QuorumCommitTopology::~QuorumCommitTopology() = default;

const DsnList& QuorumCommitTopology::GetDsnList() const {
  return pimpl_->GetDsnList();
}

rcu::ReadablePtr<QuorumCommitTopology::DsnIndicesByType>
QuorumCommitTopology::GetDsnIndicesByType() const {
  return pimpl_->GetDsnIndicesByType();
}

rcu::ReadablePtr<QuorumCommitTopology::DsnIndices>
QuorumCommitTopology::GetAliveDsnIndices() const {
  return pimpl_->GetAliveDsnIndices();
}

const std::vector<decltype(InstanceStatistics::topology)>&
QuorumCommitTopology::GetDsnStatistics() const {
  return pimpl_->GetDsnStatistics();
}

std::vector<std::string> ParseSyncStandbyNames(std::string_view value) {
  static const std::string_view kQuorumKeyword = "ANY";
  static const std::string_view kMultiKeyword = "FIRST";

  size_t num_sync = 0;
  auto token = ConsumeToken(value);
  if (::utils::StrIcaseEqual{}(token, kQuorumKeyword)) {
    // ANY num_sync ( standby_name [, ...] )
    LOG_TRACE() << "Quorum replication detected";
    // TODO?: we can check that num_sync is less than the number of standbys
  } else if (!token.empty()) {
    if (::utils::StrIcaseEqual{}(token, kMultiKeyword))
      token = ConsumeToken(value);
    if (value.find('(') != std::string_view::npos) {
      // [FIRST] num_sync ( standby_name [, ...] )
      num_sync = ParseSize(token);
      token = ConsumeToken(value);
    } else {
      // standby_name [, ...]
      num_sync = 1;
    }
  }

  std::vector<std::string> sync_slave_names;
  while (num_sync--) {
    sync_slave_names.emplace_back(token);
    token = ConsumeToken(value);
  }
  return sync_slave_names;
}

}  // namespace storages::postgres::detail
