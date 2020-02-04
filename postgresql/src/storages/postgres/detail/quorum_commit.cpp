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
#include <utils/periodic_task.hpp>
#include <utils/scope_guard.hpp>

namespace storages::postgres::detail {

namespace {

constexpr auto kCheckTimeout = std::chrono::seconds{1};
constexpr auto kDiscoveryInterval = std::chrono::seconds{1};

// Special connection ID to ease detection in logs
constexpr uint32_t kConnectionId = 4'100'200'300;
constexpr const char* kDiscoveryTaskName = "pg_topology";

const std::string kSelectSyncSlaveNames =
    R"~(select distinct application_name from pg_stat_replication where sync_state = 'sync')~";

struct HostStatus;

struct CheckStatus {
  HostStatus* host{nullptr};
  std::chrono::microseconds roundtrip_time{};
  std::vector<std::string> detected_sync_slaves{};
};

struct HostStatus {
  std::string dsn;
  // In pg_stat_replication slaves' host names are escaped and the column is
  // called `application_name`
  std::string app_name;

  std::unique_ptr<Connection> connection{};
  ClusterHostType role = ClusterHostType::kUnknown;

  explicit HostStatus(const std::string& dsn)
      : dsn{dsn}, app_name{EscapeHostName(OptionsFromDsn(dsn).host)} {}

  ~HostStatus();

  CheckStatus RunCheck(engine::TaskProcessor& tp, ConnectionSettings settings,
                       CommandControl default_cmd_ctl,
                       const error_injection::Settings& ei_settings);
};

}  // namespace

struct QuorumCommitCluster::Impl {
  Impl(engine::TaskProcessor& bg_task_processor, const DSNList& dsns,
       ConnectionSettings conn_settings, CommandControl default_cmd_ctl,
       const error_injection::Settings& ei_settings);
  ~Impl();

  void RunDiscovery();

  void StartPeriodicTask();
  void StopPeriodicTask();

  /// Background task processor passed to connection objects
  engine::TaskProcessor& bg_task_processor;
  /// All DSNs handled by this topology discovery component
  DSNList dsns_;
  /// Host states array
  std::vector<HostStatus> host_states;
  /// Individual connection settings
  ConnectionSettings conn_settings;
  CommandControl default_cmd_ctl;
  const error_injection::Settings& ei_settings;

  ::utils::PeriodicTask discovery_task;

  /// Currently determined host types exposed to the client
  rcu::Variable<HostsByType> hosts_by_type;

  /// Currently accessible hosts by roundtrip time
  rcu::Variable<DSNList> hosts_by_rtt;
};

QuorumCommitCluster::Impl::Impl(engine::TaskProcessor& bg_task_processor,
                                const DSNList& dsns,
                                ConnectionSettings conn_settings,
                                CommandControl default_cmd_ctl,
                                const error_injection::Settings& ei_settings)
    : bg_task_processor{bg_task_processor},
      dsns_{dsns},
      host_states{dsns.begin(), dsns.end()},
      conn_settings{conn_settings},
      default_cmd_ctl{default_cmd_ctl},
      ei_settings{ei_settings} {
  RunDiscovery();
  StartPeriodicTask();
}

QuorumCommitCluster::Impl::~Impl() { StopPeriodicTask(); }

void QuorumCommitCluster::Impl::StartPeriodicTask() {
  using Flags = ::utils::PeriodicTask::Flags;

  discovery_task.Start(kDiscoveryTaskName,
                       {kDiscoveryInterval, {Flags::kStrong}},
                       [this] { RunDiscovery(); });
}
void QuorumCommitCluster::Impl::StopPeriodicTask() { discovery_task.Stop(); }

void QuorumCommitCluster::Impl::RunDiscovery() {
  std::vector<engine::TaskWithResult<CheckStatus>> tasks;
  for (auto& hs : host_states) {
    tasks.emplace_back(engine::impl::Async(
        [this](HostStatus& hs) {
          return hs.RunCheck(bg_task_processor, conn_settings, default_cmd_ctl,
                             ei_settings);
        },
        std::ref(hs)));
  }
  std::vector<CheckStatus> check_statuses;
  for (auto& t : tasks) {
    auto status = t.Get();
    LOG_DEBUG() << status.host->app_name << " is " << status.host->role
                << " rtt " << status.roundtrip_time.count() << "us";
    if (status.host->role != ClusterHostType::kUnknown) {
      check_statuses.emplace_back(std::move(status));
    }
  }
  // At this stage check statuses can contain only two types of hosts - master
  // and slave. The record for master can contain names of sync slaves
  auto master = std::find_if(
      check_statuses.begin(), check_statuses.end(),
      [](const auto& s) { return s.host->role == ClusterHostType::kMaster; });

  // O(N^2), seems OK for expected number of items
  if (master != check_statuses.end() && !master->detected_sync_slaves.empty()) {
    for (const auto& ss : master->detected_sync_slaves) {
      for (auto& s : check_statuses) {
        if (s.host->app_name == ss) {
          LOG_DEBUG() << s.host->app_name << " is a sync slave";
          s.host->role = ClusterHostType::kSyncSlave;
        }
      }
    }
  }

  std::sort(check_statuses.begin(), check_statuses.end(),
            [](const auto& lhs, const auto& rhs) {
              return lhs.roundtrip_time < rhs.roundtrip_time;
            });
  HostsByType by_type;
  DSNList by_rtt;
  for (auto& hs : check_statuses) {
    by_type[hs.host->role].push_back(hs.host->dsn);
    by_rtt.push_back(hs.host->dsn);
  }
  hosts_by_type.Assign(std::move(by_type));
  hosts_by_rtt.Assign(by_rtt);
}

HostStatus::~HostStatus() {
  if (connection) {
    connection->Close();
  }
}

CheckStatus HostStatus::RunCheck(engine::TaskProcessor& tp,
                                 ConnectionSettings settings,
                                 CommandControl default_cmd_ctl,
                                 const error_injection::Settings& ei_settings) {
  ::utils::ScopeGuard role_check_guard([this] {
    role = ClusterHostType::kUnknown;
    connection.reset();
  });

  if (!connection) {
    try {
      connection =
          Connection::Connect(dsn, tp, kConnectionId, settings, default_cmd_ctl,
                              ei_settings, Connection::ConnToken{});
    } catch (const ConnectionError& e) {
      LOG_ERROR() << "Failed to connect to " << DsnCutPassword(dsn) << ": "
                  << e;
      return {this, std::chrono::microseconds{0}};
    }
  }
  auto deadline = engine::Deadline::FromDuration(kCheckTimeout);
  auto start = std::chrono::system_clock::now();
  try {
    auto ro = connection->CheckReadOnly(deadline);
    role = ro ? ClusterHostType::kSlave : ClusterHostType::kMaster;
    auto rtt = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now() - start);
    CheckStatus status{this, rtt};
    if (role == ClusterHostType::kMaster) {
      auto res = connection->Execute(kSelectSyncSlaveNames);
      LOG_DEBUG() << res.Size() << " sync slaves detected";
      status.detected_sync_slaves = res.AsContainer<std::vector<std::string>>();
    }
    role_check_guard.Release();
    return status;
  } catch (const ConnectionError& e) {
    LOG_ERROR() << "Broken connection with " << DsnCutPassword(dsn) << ": "
                << e;
  }
  return {this, std::chrono::microseconds{0}};
}

QuorumCommitCluster::QuorumCommitCluster(
    engine::TaskProcessor& bg_task_processor, const DSNList& dsns,
    ConnectionSettings conn_settings, CommandControl default_cmd_ctl,
    const error_injection::Settings& ei_settings)
    : pimpl_(bg_task_processor, dsns, conn_settings, default_cmd_ctl,
             ei_settings){};

QuorumCommitCluster::~QuorumCommitCluster() = default;

const DSNList& QuorumCommitCluster::GetDsnList() const { return pimpl_->dsns_; }

QuorumCommitCluster::HostsByType QuorumCommitCluster::GetHostsByType() const {
  auto ptr = pimpl_->hosts_by_type.Read();
  return *ptr;
}

DSNList QuorumCommitCluster::GetHostsByRoundtripTime() const {
  auto ptr = pimpl_->hosts_by_rtt.Read();
  return *ptr;
}

}  // namespace storages::postgres::detail
