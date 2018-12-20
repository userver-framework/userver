#include <storages/postgres/detail/topology_discovery.hpp>

#include <engine/async.hpp>
#include <engine/sleep.hpp>
#include <logging/log.hpp>
#include <storages/postgres/exceptions.hpp>

namespace storages {
namespace postgres {
namespace detail {

namespace {

constexpr const char* kPeriodicTaskName = "pg_topology";
// ------------------------------------------------------------
// TODO Move constants to config
const std::chrono::seconds kUpdateInterval(5);
constexpr size_t kImmediateReconnects = 5;
const std::chrono::seconds kReconnectInterval(3);
// ------------------------------------------------------------
constexpr ClusterHostType kNothing = static_cast<ClusterHostType>(0);
constexpr size_t kInvalidIndex = static_cast<size_t>(-1);
constexpr uint32_t kConnectionId = 4'100'200'300;

std::string HostAndPortFromDsn(const std::string& dsn) {
  auto options = OptionsFromDsn(dsn);
  return options.host + ':' + options.port;
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

ClusterTopologyDiscovery::ClusterTopologyDiscovery(
    engine::TaskProcessor& bg_task_processor, const DSNList& dsn_list)
    : bg_task_processor_(bg_task_processor), update_lock_ ATOMIC_FLAG_INIT {
  CreateConnections(dsn_list);
  BuildEscapedNames();
  StartPeriodicUpdates();
}

ClusterTopologyDiscovery::~ClusterTopologyDiscovery() { StopRunningTasks(); }

ClusterTopology::HostsByType ClusterTopologyDiscovery::GetHostsByType() const {
  std::lock_guard<engine::Mutex> lock(hosts_mutex_);
  const auto host_count = connections_.size();
  assert(host_count == host_types_.size() &&
         "DSN count and host types count don't match");

  // TODO consider using SwappingSmart
  HostsByType hosts_by_type;
  for (size_t i = 0; i < host_count; ++i) {
    const auto host_type = host_types_[i];
    if (host_type != kNothing) {
      hosts_by_type[host_type].push_back(connections_[i].dsn);
    }
  }
  return hosts_by_type;
}

void ClusterTopologyDiscovery::BuildEscapedNames() {
  const auto host_count = connections_.size();
  host_types_.resize(host_count, kNothing);

  for (size_t i = 0; i < host_count; ++i) {
    const auto options = OptionsFromDsn(connections_[i].dsn);
    escaped_to_dsn_index_[EscapeHostName(options.host)] = i;
  }
}

void ClusterTopologyDiscovery::CreateConnections(const DSNList& dsn_list) {
  const auto host_count = dsn_list.size();
  std::vector<engine::TaskWithResult<ConnectionPtr>> tasks;
  tasks.reserve(host_count);

  LOG_INFO() << "Creating connections to monitor cluster topology";
  for (auto&& dsn : dsn_list) {
    tasks.push_back(Connect(dsn));
  }

  // Wait for connections to be established, but grab them when they are needed
  // This way we don't need to handle connection errors in place
  for (auto&& task : tasks) {
    task.Wait();
  }

  connections_.reserve(host_count);
  for (size_t i = 0; i < host_count; ++i) {
    connections_.push_back(
        ConnectionState{dsn_list[i], std::move(tasks[i]), 0});
  }
}

void ClusterTopologyDiscovery::StartPeriodicUpdates() {
  using ::utils::PeriodicTask;
  using Flags = ::utils::PeriodicTask::Flags;

  PeriodicTask::Settings settings(kUpdateInterval, {Flags::kNow});
  periodic_task_.Start(kPeriodicTaskName, settings,
                       [this] { CheckTopology(); });
}

void ClusterTopologyDiscovery::StopRunningTasks() {
  periodic_task_.Stop();
  LOG_INFO() << "Closing connections";
  for (auto&& conn : connections_) {
    try {
      CloseConnection(std::move(boost::get<ConnectionPtr>(conn.conn_variant)));
    } catch (const boost::bad_get&) {
      auto& conn_task = boost::get<ConnectionTask>(conn.conn_variant);
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
  const auto failed_reconnects = connections_[index].failed_reconnects++;
  auto conn =
      std::move(boost::get<ConnectionPtr>(connections_[index].conn_variant));
  if (conn) {
    LOG_DEBUG() << conn->GetLogExtra() << "Starting reconnect #"
                << failed_reconnects + 1;
  } else {
    LOG_DEBUG() << "Starting reconnect #" << failed_reconnects + 1
                << " for host=" << HostAndPortFromDsn(connections_[index].dsn);
  }

  auto task = engine::Async(
      [this, failed_reconnects](ConnectionPtr conn, std::string dsn) {
        const auto wait_for_reconnect =
            failed_reconnects >= kImmediateReconnects;
        std::chrono::system_clock::time_point tp;
        if (wait_for_reconnect) {
          tp = std::chrono::system_clock::now() + kReconnectInterval;
        }

        CloseConnection(std::move(conn));

        if (wait_for_reconnect) {
          engine::SleepUntil(tp);
        }
        return Connect(std::move(dsn)).Get();
      },
      std::move(conn), connections_[index].dsn);
  connections_[index].conn_variant = std::move(task);
}

void ClusterTopologyDiscovery::CloseConnection(ConnectionPtr conn_ptr) {
  if (conn_ptr) {
    conn_ptr->Close();
  }
}

Connection* ClusterTopologyDiscovery::GetConnectionOrThrow(size_t index) const
    noexcept(false) {
  return boost::get<ConnectionPtr>(connections_[index].conn_variant).get();
}

Connection* ClusterTopologyDiscovery::GetConnectionOrNull(size_t index) {
  try {
    return GetConnectionOrThrow(index);
  } catch (const boost::bad_get&) {
  }

  auto& conn_task =
      boost::get<ConnectionTask>(connections_[index].conn_variant);
  if (!conn_task.IsFinished()) {
    return nullptr;
  }

  try {
    auto conn_ptr = conn_task.Get();
    auto* conn = conn_ptr.get();
    connections_[index].conn_variant = std::move(conn_ptr);
    connections_[index].failed_reconnects = 0;
    return conn;
  } catch (const ConnectionError&) {
    // Reconnect expects connection rather than task
    connections_[index].conn_variant = nullptr;
    Reconnect(index);
  }
  return nullptr;
}

void ClusterTopologyDiscovery::CheckTopology() {
  HostTypeList host_types;
  {
    TryLockGuard lock(update_lock_);
    if (!lock.LockAcquired()) {
      LOG_TRACE() << "Already checking cluster topology";
      return;
    }

    // TODO check for close updates and discard those
    LOG_INFO() << "Checking cluster topology";

    host_types.resize(connections_.size(), kNothing);
    const auto master_index = CheckHostsAndFindMaster(host_types);
    FindSyncSlaves(host_types, master_index);
  }

  LOG_TRACE() << DumpTopologyState(host_types);

  std::lock_guard<engine::Mutex> lock(hosts_mutex_);
  host_types_ = std::move(host_types);
}

size_t ClusterTopologyDiscovery::CheckHostsAndFindMaster(
    HostTypeList& host_types) {
  const auto host_count = connections_.size();
  std::vector<std::pair<size_t, engine::TaskWithResult<ClusterHostType>>> tasks;
  tasks.reserve(host_count);

  for (size_t i = 0; i < host_count; ++i) {
    auto* conn = GetConnectionOrNull(i);
    if (!conn) {
      // TODO maybe wait for some time now?
      continue;
    }

    auto task = engine::Async([conn] {
      auto res = conn->Execute("select pg_is_in_recovery()");
      assert(!res.IsEmpty() && "pg_is_in_recovery must return bool value");

      bool in_recovery = true;
      res.Front().To(in_recovery);
      return in_recovery ? ClusterHostType::kSlave : ClusterHostType::kMaster;
    });
    tasks.push_back(std::make_pair(i, std::move(task)));
  }

  size_t master_index = kInvalidIndex;
  for (auto && [ index, task ] : tasks) {
    auto host_type = kNothing;
    try {
      host_type = task.Get();
    } catch (const ConnectionError&) {
      Reconnect(index);
    }
    host_types[index] = host_type;

    assert(host_type != kNothing && "Wrong replica state received");
    if (host_type == ClusterHostType::kMaster) {
      if (master_index != kInvalidIndex) {
        LOG_WARNING() << "More than one master host found";
      }
      master_index = index;
    }
  }
  return master_index;
}

void ClusterTopologyDiscovery::FindSyncSlaves(HostTypeList& host_types,
                                              size_t master_index) {
  if (master_index == kInvalidIndex) {
    LOG_WARNING() << "No master hosts found";
    return;
  }

  auto* conn = GetConnectionOrThrow(master_index);
  LOG_INFO() << conn->GetLogExtra() << "Found master host";
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

  std::vector<size_t> sync_slave_indices;
  try {
    sync_slave_indices = task.Get();
  } catch (const ConnectionError&) {
    LOG_WARNING() << "Master host is lost while asking for sync slaves";
    host_types[master_index] = kNothing;
    Reconnect(master_index);
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
      host_types[index] = ClusterHostType::kSyncSlave;
    } else {
      assert(host_types[index] == kNothing &&
             "Missing host should already be marked as kNothing");
      LOG_WARNING() << "Found unavailable sync slave host="
                    << HostAndPortFromDsn(connections_[index].dsn);
    }
  }
}

std::string ClusterTopologyDiscovery::DumpTopologyState(
    const HostTypeList& host_types) const {
  const auto host_count = connections_.size();
  assert(host_count == host_types.size() &&
         "DSN count and host types count don't match");

  std::string topology_state = "Topology state:\n";
  for (size_t i = 0; i < host_count; ++i) {
    const auto host_type = host_types[i];
    std::string host_type_name =
        (host_type != kNothing) ? ToString(host_type) : "--- unavailable ---";
    topology_state += HostAndPortFromDsn(connections_[i].dsn) + " : " +
                      std::move(host_type_name) + '\n';
  }
  return topology_state;
}

}  // namespace detail
}  // namespace postgres
}  // namespace storages
