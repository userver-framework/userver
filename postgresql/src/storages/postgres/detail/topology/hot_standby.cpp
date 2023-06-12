#include <storages/postgres/detail/topology/hot_standby.hpp>

#include <algorithm>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include <crypto/openssl.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/internal_pg_types.hpp>
#include <storages/postgres/io/pg_type_parsers.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/periodic_task.hpp>
#include <userver/utils/str_icase.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail::topology {

namespace {

constexpr auto kCheckTimeout = std::chrono::seconds{1};
constexpr auto kDiscoveryInterval = std::chrono::seconds{1};

using Rtt = std::chrono::microseconds;
constexpr Rtt kUnknownRtt{-1};

using ReplicationLag = std::chrono::milliseconds;

constexpr const char* kDiscoveryTaskName = "pg_topology";

const std::string kShowSyncStandbyNames = "SHOW synchronous_standby_names";

struct WalInfoStatements {
  int min_version;
  std::string master;
  std::string slave;
};

const WalInfoStatements& GetWalInfoStatementsForVersion(int version) {
  // must be in descending min_version order
  static const WalInfoStatements kKnownStatements[]{
      // Master doesn't provide last xact timestamp without
      // `track_commit_timestamp` enabled, use current server time
      // as an approximation.
      // Also note that all times here are sourced from the master.
      {100000, "SELECT pg_current_wal_lsn(), now()",
       "SELECT pg_last_wal_replay_lsn(), pg_last_xact_replay_timestamp()"},

      // Versions for 9.4-9.x servers.
      // (Functions were actually available since 8.2 but returned TEXT.)
      {90400, "SELECT pg_current_xlog_location(), now()",
       "SELECT pg_last_xlog_replay_location(), "
       "pg_last_xact_replay_timestamp()"}};

  for (const auto& cand : kKnownStatements) {
    if (version >= cand.min_version) return cand;
  }
  throw PoolError{fmt::format("Unsupported database version: {}", version)};
}

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

struct HotStandby::HostState {
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

HotStandby::HotStandby(engine::TaskProcessor& bg_task_processor, DsnList dsns,
                       clients::dns::Resolver* resolver,
                       const TopologySettings& topology_settings,
                       const ConnectionSettings& conn_settings,
                       const DefaultCommandControls& default_cmd_ctls,
                       const testsuite::PostgresControl& testsuite_pg_ctl,
                       error_injection::Settings ei_settings)
    : TopologyBase(bg_task_processor, std::move(dsns), resolver,
                   topology_settings, conn_settings, default_cmd_ctls,
                   testsuite_pg_ctl, std::move(ei_settings)),
      host_states_{GetDsnList().begin(), GetDsnList().end()},
      dsn_stats_(GetDsnList().size()) {
  crypto::impl::Openssl::Init();
  RunDiscovery();

  discovery_task_.Start(
      kDiscoveryTaskName,
      {kDiscoveryInterval,
       {USERVER_NAMESPACE::utils::PeriodicTask::Flags::kStrong}},
      [this] { RunDiscovery(); });
}

HotStandby::~HotStandby() { discovery_task_.Stop(); }

rcu::ReadablePtr<TopologyBase::DsnIndicesByType>
HotStandby::GetDsnIndicesByType() const {
  return dsn_indices_by_type_.Read();
}

rcu::ReadablePtr<TopologyBase::DsnIndices> HotStandby::GetAliveDsnIndices()
    const {
  return alive_dsn_indices_.Read();
}

const std::vector<decltype(InstanceStatistics::topology)>&
HotStandby::GetDsnStatistics() const {
  return dsn_stats_;
}

void HotStandby::RunDiscovery() {
  std::vector<engine::TaskWithResult<void>> tasks;
  tasks.reserve(GetDsnList().size());
  for (DsnIndex i = 0; i < GetDsnList().size(); ++i) {
    tasks.emplace_back(engine::AsyncNoSpan([this, i] { RunCheck(i); }));
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

    if (slave_lag > GetTopologySettings().max_replication_lag) {
      // Demote lagged slave
      LOG_INFO() << "Disabling slave " << slave.app_name
                 << " due to replication lag of " << slave_lag.count()
                 << " ms (max "
                 << GetTopologySettings().max_replication_lag.count() << " ms)";
      slave.role = ClusterHostType::kNone;
    } else if (master) {
      // Check for sync slave
      // O(N^2), seems OK for expected number of items
      for (const auto& ss_app_name : master->detected_sync_slaves) {
        if (USERVER_NAMESPACE::utils::StrIcaseEqual{}(slave.app_name,
                                                      ss_app_name)) {
          LOG_DEBUG() << slave.app_name << " is a sync slave";
          slave.role = ClusterHostType::kSyncSlave;
        }
      }
    }
  }

  // Demote readonly master to slave
  if (master && master->is_readonly) {
    if (!GetTestsuiteControl().IsReadonlyMasterExpected()) {
      LOG_WARNING() << "Primary host is not writable, possibly due to "
                       "insufficient disk space";
    }
    master->role = ClusterHostType::kSlave;
  }

  DsnIndices alive_dsn_indices;
  for (DsnIndex i = 0; i < GetDsnList().size(); ++i) {
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

void HotStandby::RunCheck(DsnIndex idx) {
  UASSERT(idx < GetDsnList().size());
  const auto& dsn = GetDsnList()[idx];
  auto& state = host_states_[idx];

  USERVER_NAMESPACE::utils::FastScopeGuard role_check_guard(
      [&state]() noexcept { state.Reset(); });

  if (!state.connection) {
    try {
      state.connection = MakeTopologyConnection(idx);
    } catch (const ConnectionError& e) {
      LOG_WARNING() << "Failed to connect to " << DsnCutPassword(dsn) << ": "
                    << e;
      return;
    }
  }
  auto deadline = GetTestsuiteControl().MakeExecuteDeadline(kCheckTimeout);
  auto start = std::chrono::steady_clock::now();
  try {
    state.connection->RefreshReplicaState(deadline);
    state.role = state.connection->IsInRecovery() ? ClusterHostType::kSlave
                                                  : ClusterHostType::kMaster;
    state.is_readonly = state.connection->IsReadOnly();
    state.roundtrip_time = std::chrono::duration_cast<Rtt>(
        std::chrono::steady_clock::now() - start);

    const auto& wal_info_stmts =
        GetWalInfoStatementsForVersion(state.connection->GetServerVersion());
    std::optional<std::chrono::system_clock::time_point> current_xact_timestamp;

    const auto wal_info = state.connection->Execute(
        state.connection->IsInRecovery() ? wal_info_stmts.slave
                                         : wal_info_stmts.master);
    wal_info.Front().To(state.wal_lsn, current_xact_timestamp);
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
    LOG_LIMITED_WARNING() << "Broken connection with " << DsnCutPassword(dsn)
                          << ": " << e;
  }
}

std::vector<std::string> ParseSyncStandbyNames(std::string_view value) {
  static const std::string_view kQuorumKeyword = "ANY";
  static const std::string_view kMultiKeyword = "FIRST";

  size_t num_sync = 0;
  auto token = ConsumeToken(value);
  if (USERVER_NAMESPACE::utils::StrIcaseEqual{}(token, kQuorumKeyword)) {
    // ANY num_sync ( standby_name [, ...] )
    LOG_TRACE() << "Quorum replication detected";
    // TODO?: we can check that num_sync is less than the number of standbys
  } else if (!token.empty()) {
    if (USERVER_NAMESPACE::utils::StrIcaseEqual{}(token, kMultiKeyword))
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

}  // namespace storages::postgres::detail::topology

USERVER_NAMESPACE_END
