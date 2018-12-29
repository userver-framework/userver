#include <storages/postgres/detail/topology_discovery.hpp>

#include <engine/async.hpp>
#include <engine/sleep.hpp>
#include <logging/log.hpp>
#include <storages/postgres/exceptions.hpp>

namespace storages {
namespace postgres {
namespace detail {

namespace {

// ------------------------------------------------------------
// TODO Move constants to config
// How many immediate reconnect attempts are tried after connection failure
constexpr size_t kImmediateReconnects = 2;
// Interval between reconnect attempts after kImmediateReconnects tries
const std::chrono::seconds kReconnectInterval(3);
// TODO Do we need this?
// Failed operations count after which the host is marked as unavailable
constexpr size_t kFailureThreshold = 30;
// State (type/availability) of a host is changed when this many subsequent
// identical new states observed
constexpr size_t kCooldownThreshold = 3;
// ------------------------------------------------------------
// Constant marking unavailable host type
constexpr ClusterHostType kNothing = static_cast<ClusterHostType>(0);
// Invalid index
constexpr size_t kInvalidIndex = static_cast<size_t>(-1);
// Special connection ID to ease detection in logs
constexpr uint32_t kConnectionId = 4'100'200'300;
// Time slice used in task polling checks
const std::chrono::milliseconds kWaitInterval(100);
// Minimal duration of topology check routine
const std::chrono::milliseconds kMinCheckDuration(3000);

std::string HostAndPortFromDsn(const std::string& dsn) {
  auto options = OptionsFromDsn(dsn);
  return options.host + ':' + options.port;
}

using TaskList = std::vector<engine::Task*>;

size_t WaitAnyUntil(const TaskList& tasks,
                    const std::chrono::steady_clock::time_point& time_point) {
  do {
    for (size_t i = 0; i < tasks.size(); ++i) {
      const auto* task = tasks[i];
      if (!task || !task->IsValid()) {
        continue;
      }

      if (task->IsFinished()) {
        return i;
      }
    }

    const auto next_point =
        std::min(std::chrono::steady_clock::now() + kWaitInterval, time_point);
    engine::SleepUntil(next_point);
  } while (std::chrono::steady_clock::now() < time_point);

  return kInvalidIndex;
}

std::string HostTypeToString(ClusterHostType host_type) {
  return host_type != kNothing ? ToString(host_type) : "--- unavailable ---";
}

struct TryLockGuard {
  TryLockGuard(std::atomic_flag& lock) : lock_(lock) {
    lock_acquired_ = !lock_.test_and_set(std::memory_order_acq_rel);
  }

  ~TryLockGuard() {
    if (lock_acquired_) {
      lock_.clear(std::memory_order_release);
    }
  }

  bool LockAcquired() const { return lock_acquired_; }

 private:
  std::atomic_flag& lock_;
  bool lock_acquired_;
};

}  // namespace

const std::chrono::seconds ClusterTopologyDiscovery::kUpdateInterval{5};

ClusterTopologyDiscovery::HostState::HostState(const std::string& dsn,
                                               ConnectionTask&& task)
    : dsn(dsn),
      conn_variant(std::move(task)),
      host_type(kNothing),
      failed_reconnects(0),
      changes{kNothing, 0},
      checks{{}, HostChecks::Stage::kReconnect},
      failed_operations(0) {}

ClusterTopologyDiscovery::ClusterTopologyDiscovery(
    engine::TaskProcessor& bg_task_processor, const DSNList& dsn_list)
    : bg_task_processor_(bg_task_processor),
      check_duration_(std::chrono::duration_cast<std::chrono::milliseconds>(
                          kUpdateInterval) *
                      4 / 5),
      initial_check_(true),
      update_lock_ ATOMIC_FLAG_INIT {
  if (check_duration_ < kMinCheckDuration) {
    check_duration_ = kMinCheckDuration;
    LOG_WARNING() << "Too short topology update interval specified. Topology "
                     "check duration is set to "
                  << check_duration_.count() << " ms";
  }
  CreateConnections(dsn_list);
  BuildIndexes();
}

ClusterTopologyDiscovery::~ClusterTopologyDiscovery() { StopRunningTasks(); }

ClusterTopology::HostsByType ClusterTopologyDiscovery::GetHostsByType() const {
  std::lock_guard<engine::Mutex> lock(hosts_mutex_);
  return hosts_by_type_;
}

void ClusterTopologyDiscovery::BuildIndexes() {
  const auto host_count = host_states_.size();
  dsn_to_index_.reserve(host_count);
  escaped_to_dsn_index_.reserve(host_count);

  for (size_t i = 0; i < host_count; ++i) {
    // Build name to index mapping
    dsn_to_index_[host_states_[i].dsn] = i;
    // Build escaped name to index mapping
    const auto options = OptionsFromDsn(host_states_[i].dsn);
    escaped_to_dsn_index_[EscapeHostName(options.host)] = i;
  }
}

void ClusterTopologyDiscovery::CreateConnections(const DSNList& dsn_list) {
  LOG_INFO() << "Creating connections to monitor cluster topology";

  host_states_.reserve(dsn_list.size());
  for (auto&& dsn : dsn_list) {
    // Don't wait for connections, we'll do it during topology check
    host_states_.emplace_back(dsn, Connect(dsn));
  }
}

void ClusterTopologyDiscovery::StopRunningTasks() {
  LOG_INFO() << "Closing connections";
  for (auto&& state : host_states_) {
    try {
      CloseConnection(std::move(boost::get<ConnectionPtr>(state.conn_variant)));
    } catch (const boost::bad_get&) {
      auto& conn_task = boost::get<ConnectionTask>(state.conn_variant);
      conn_task.RequestCancel();
      conn_task = ConnectionTask();
    }
  }
  LOG_INFO() << "Closed connections";
}

engine::TaskWithResult<ConnectionPtr> ClusterTopologyDiscovery::Connect(
    std::string dsn) {
  return engine::Async([ this, dsn = std::move(dsn) ] {
    ConnectionPtr conn =
        Connection::Connect(dsn, bg_task_processor_, kConnectionId);
    return conn;
  });
}

void ClusterTopologyDiscovery::Reconnect(size_t index) {
  AccountHostTypeChange(index, kNothing);
  const auto failed_reconnects = host_states_[index].failed_reconnects++;
  auto conn =
      std::move(boost::get<ConnectionPtr>(host_states_[index].conn_variant));
  if (conn) {
    LOG_DEBUG() << conn->GetLogExtra() << "Starting reconnect #"
                << failed_reconnects + 1;
  } else {
    LOG_DEBUG() << "Starting reconnect #" << failed_reconnects + 1
                << " for host=" << HostAndPortFromDsn(host_states_[index].dsn);
  }

  auto task = engine::Async(
      [this, failed_reconnects](ConnectionPtr conn, std::string dsn) {
        const auto wait_for_reconnect =
            failed_reconnects >= kImmediateReconnects;
        std::chrono::steady_clock::time_point tp;
        if (wait_for_reconnect) {
          tp = std::chrono::steady_clock::now() + kReconnectInterval;
        }

        CloseConnection(std::move(conn));

        if (wait_for_reconnect) {
          engine::SleepUntil(tp);
        }
        return Connect(std::move(dsn)).Get();
      },
      std::move(conn), host_states_[index].dsn);
  host_states_[index].conn_variant = std::move(task);
  host_states_[index].checks.stage = HostChecks::Stage::kReconnect;
}

void ClusterTopologyDiscovery::CloseConnection(ConnectionPtr conn_ptr) {
  if (conn_ptr) {
    conn_ptr->Close();
  }
}

void ClusterTopologyDiscovery::AccountHostTypeChange(size_t index,
                                                     ClusterHostType new_type) {
  if (new_type == host_states_[index].host_type) {
    // Same type as we have now
    host_states_[index].changes.count = 0;
  } else if (new_type == host_states_[index].changes.new_type) {
    // Subsequent host type change
    ++host_states_[index].changes.count;
  } else {
    // Brand new host type change
    host_states_[index].changes.count = 1;
  }
  host_states_[index].changes.new_type = new_type;
}

bool ClusterTopologyDiscovery::ShouldChangeHostType(size_t index) const {
  // We want to publish availability states after the very first check
  if (initial_check_) {
    return true;
  }
  // Immediately change host type if there is too many failed operations
  if (host_states_[index].failed_operations >= kFailureThreshold) {
    return true;
  }
  if (host_states_[index].changes.count >= kCooldownThreshold) {
    return true;
  }
  return false;
}

Connection* ClusterTopologyDiscovery::GetConnectionOrThrow(size_t index) const
    noexcept(false) {
  return boost::get<ConnectionPtr>(host_states_[index].conn_variant).get();
}

Connection* ClusterTopologyDiscovery::GetConnectionOrNull(size_t index) {
  try {
    return GetConnectionOrThrow(index);
  } catch (const boost::bad_get&) {
  }

  auto& conn_task =
      boost::get<ConnectionTask>(host_states_[index].conn_variant);
  if (!conn_task.IsFinished()) {
    return nullptr;
  }

  try {
    auto conn_ptr = conn_task.Get();
    auto* conn = conn_ptr.get();
    host_states_[index].conn_variant = std::move(conn_ptr);
    host_states_[index].failed_reconnects = 0;
    return conn;
  } catch (const ConnectionError&) {
    // Reconnect expects connection rather than task
    host_states_[index].conn_variant = nullptr;
    Reconnect(index);
  }
  return nullptr;
}

void ClusterTopologyDiscovery::CheckTopology() {
  HostsByType hosts_by_type;

  {
    TryLockGuard lock(update_lock_);
    if (!lock.LockAcquired()) {
      LOG_TRACE() << "Already checking cluster topology";
      return;
    }

    const auto check_end_point =
        std::chrono::steady_clock::now() + check_duration_;
    LOG_INFO() << "Checking cluster topology. Check duration is "
               << check_duration_.count() << " ms";
    CheckHosts(check_end_point);

    const auto updated = UpdateHostTypes();
    if (updated) {
      hosts_by_type = BuildHostsByType();
    }
  }

  LOG_TRACE() << DumpTopologyState();

  if (!hosts_by_type.empty()) {
    std::lock_guard<engine::Mutex> lock(hosts_mutex_);
    // TODO consider using SwappingSmart
    hosts_by_type_ = std::move(hosts_by_type);
  }
}

void ClusterTopologyDiscovery::OperationFailed(const std::string& dsn) {
  const auto index = dsn_to_index_[dsn];
  ++host_states_[index].failed_operations;
}

void ClusterTopologyDiscovery::CheckHosts(
    const std::chrono::steady_clock::time_point& check_end_point) {
  const auto host_count = host_states_.size();
  TaskList check_tasks(host_count, nullptr);

  for (size_t i = 0; i < host_count; ++i) {
    check_tasks[i] = CheckAvailability(i);
  }

  size_t index = kInvalidIndex;
  while ((index = WaitAnyUntil(check_tasks, check_end_point)) !=
         kInvalidIndex) {
    switch (host_states_[index].checks.stage) {
      case HostChecks::Stage::kReconnect:
        check_tasks[index] = CheckAvailability(index);
        break;
      case HostChecks::Stage::kAvailability:
        check_tasks[index] = CheckIfMaster(
            index, static_cast<engine::TaskWithResult<ClusterHostType>&>(
                       *check_tasks[index]));
        break;
      case HostChecks::Stage::kSyncSlaves:
        check_tasks[index] = CheckSyncSlaves(
            index, static_cast<engine::TaskWithResult<std::vector<size_t>>&>(
                       *check_tasks[index]));
        break;
    }
  }
}

engine::Task* ClusterTopologyDiscovery::CheckAvailability(size_t index) {
  auto* conn = GetConnectionOrNull(index);
  if (!conn) {
    assert(host_states_[index].checks.stage == HostChecks::Stage::kReconnect &&
           "Wrong host check stage");
    return &boost::get<ConnectionTask>(host_states_[index].conn_variant);
  }

  auto task = engine::Async([conn] {
    auto res = conn->Execute("select pg_is_in_recovery()");
    assert(!res.IsEmpty() && "pg_is_in_recovery must return bool value");

    const bool in_recovery = res.Front().As<bool>();
    return in_recovery ? ClusterHostType::kSlave : ClusterHostType::kMaster;
  });

  host_states_[index].checks.task =
      std::make_unique<engine::TaskWithResult<ClusterHostType>>(
          std::move(task));
  host_states_[index].checks.stage = HostChecks::Stage::kAvailability;
  return host_states_[index].checks.task.get();
}

engine::Task* ClusterTopologyDiscovery::CheckIfMaster(
    size_t index, engine::TaskWithResult<ClusterHostType>& task) {
  auto host_type = kNothing;
  try {
    host_type = task.Get();
  } catch (const ConnectionError&) {
    Reconnect(index);
    return &boost::get<ConnectionTask>(host_states_[index].conn_variant);
  }
  assert(host_type != kNothing && "Wrong replica state received");

  AccountHostTypeChange(index, host_type);
  // As the host is back online we can reset failed operations counter
  host_states_[index].failed_operations.Store(0);

  if (host_type == ClusterHostType::kMaster) {
    auto* conn = GetConnectionOrThrow(index);
    LOG_INFO() << conn->GetLogExtra() << "Found master host";
    return FindSyncSlaves(index, conn);
  }
  return nullptr;
}

engine::Task* ClusterTopologyDiscovery::FindSyncSlaves(size_t master_index,
                                                       Connection* conn) {
  auto task = engine::Async([this, conn] {
    auto res = conn->Execute("show synchronous_standby_names");
    if (res.IsEmpty()) {
      return std::vector<size_t>{};
    }

    std::vector<size_t> sync_slave_indices;
    sync_slave_indices.reserve(res.Size());
    for (auto&& res_row : res) {
      const auto sync_slave_name = res_row.As<std::string>();
      auto find_it = escaped_to_dsn_index_.find(sync_slave_name);
      if (find_it != escaped_to_dsn_index_.end()) {
        sync_slave_indices.push_back(find_it->second);
      } else {
        LOG_WARNING() << "Host index not found for sync slave name: "
                      << sync_slave_name;
      }
    }
    return sync_slave_indices;
  });

  host_states_[master_index].checks.task =
      std::make_unique<engine::TaskWithResult<std::vector<size_t>>>(
          std::move(task));
  host_states_[master_index].checks.stage = HostChecks::Stage::kSyncSlaves;
  return host_states_[master_index].checks.task.get();
}

engine::Task* ClusterTopologyDiscovery::CheckSyncSlaves(
    size_t master_index, engine::TaskWithResult<std::vector<size_t>>& task) {
  std::vector<size_t> sync_slave_indices;
  try {
    sync_slave_indices = task.Get();
  } catch (const ConnectionError&) {
    LOG_WARNING() << "Master host is lost while asking for sync slaves";
    Reconnect(master_index);
    return &boost::get<ConnectionTask>(host_states_[master_index].conn_variant);
  }

  if (sync_slave_indices.empty()) {
    LOG_WARNING() << "No sync slave hosts found";
  }
  for (auto&& index : sync_slave_indices) {
    const auto* conn = GetConnectionOrNull(index);
    if (conn) {
      LOG_INFO() << conn->GetLogExtra() << "Found sync slave host";
      if (index == master_index) {
        LOG_ERROR() << conn->GetLogExtra()
                    << "Attempt to overwrite master type with sync slave type";
      }
      AccountHostTypeChange(index, ClusterHostType::kSyncSlave);
    } else {
      LOG_WARNING() << "Found unavailable sync slave host="
                    << HostAndPortFromDsn(host_states_[index].dsn);
    }
  }
  // Nothing more to do
  return nullptr;
}

bool ClusterTopologyDiscovery::UpdateHostTypes() {
  bool updated = false;
  for (size_t i = 0; i < host_states_.size(); ++i) {
    if (!ShouldChangeHostType(i)) {
      continue;
    }
    auto& state = host_states_[i];
    const auto new_type = state.changes.new_type;
    auto* conn = GetConnectionOrNull(i);
    if (conn) {
      LOG_DEBUG() << conn->GetLogExtra() << "Host type changed from "
                  << HostTypeToString(state.host_type) << " to "
                  << HostTypeToString(new_type);
    } else {
      LOG_DEBUG() << "Host type changed from "
                  << HostTypeToString(state.host_type) << " to "
                  << HostTypeToString(new_type)
                  << " for host=" << HostAndPortFromDsn(host_states_[i].dsn);
    }
    updated = true;
    state.host_type = new_type;
    state.changes.new_type = kNothing;
    state.changes.count = 0;
  }
  initial_check_ = false;
  return updated;
}

std::string ClusterTopologyDiscovery::DumpTopologyState() const {
  std::string topology_state = "Topology state:\n";
  for (const auto& state : host_states_) {
    const auto host_type = state.host_type;
    std::string host_type_name = HostTypeToString(host_type);
    topology_state += HostAndPortFromDsn(state.dsn) + " : " +
                      std::move(host_type_name) + '\n';
  }
  return topology_state;
}

ClusterTopology::HostsByType ClusterTopologyDiscovery::BuildHostsByType()
    const {
  size_t master_index = kInvalidIndex;
  HostsByType hosts_by_type;

  for (size_t i = 0; i < host_states_.size(); ++i) {
    const auto& state = host_states_[i];
    const auto host_type = state.host_type;
    if (host_type != kNothing) {
      hosts_by_type[host_type].push_back(state.dsn);
      if (host_type == ClusterHostType::kMaster) {
        if (master_index != kInvalidIndex) {
          LOG_WARNING() << "More than one master host found";
        }
        master_index = i;
      }
    }
  }

  if (master_index == kInvalidIndex) {
    LOG_WARNING() << "No master hosts found";
  }
  return hosts_by_type;
}

}  // namespace detail
}  // namespace postgres
}  // namespace storages
