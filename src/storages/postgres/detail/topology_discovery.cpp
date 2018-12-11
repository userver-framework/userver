#include <storages/postgres/detail/topology_discovery.hpp>

#include <engine/async.hpp>
#include <logging/log.hpp>
#include <storages/postgres/exceptions.hpp>

namespace storages {
namespace postgres {
namespace detail {

namespace {

constexpr const char* kPeriodicTaskName = "pg_topology";
// TODO Move interval constant to config
const std::chrono::seconds kInterval(5);
constexpr ClusterHostType kNothing = static_cast<ClusterHostType>(0);
constexpr size_t kInvalidIndex = static_cast<size_t>(-1);
constexpr uint32_t kConnectionId = 4'100'200'300;

}  // namespace

ClusterTopologyDiscovery::ClusterTopologyDiscovery(
    engine::TaskProcessor& bg_task_processor, const DSNList& dsn_list)
    : bg_task_processor_(bg_task_processor) {
  InitHostInfo(dsn_list);
  CreateConnections();
  StartPeriodicUpdates();
}

ClusterTopologyDiscovery::~ClusterTopologyDiscovery() { periodic_task_.Stop(); }

ClusterTopology::HostsByType ClusterTopologyDiscovery::GetHostsByType() const {
  std::lock_guard<engine::Mutex> lock(hosts_mutex_);
  assert(dsn_list_.size() == host_types_.size() &&
         "DSN count and host types count don't match");

  // TODO consider using SwappingSmart
  HostsByType hosts_by_type;
  for (size_t i = 0; i < dsn_list_.size(); ++i) {
    const auto host_type = host_types_[i];
    if (host_type != kNothing) {
      hosts_by_type[host_type].push_back(dsn_list_[i]);
    }
  }
  return hosts_by_type;
}

void ClusterTopologyDiscovery::InitHostInfo(const DSNList& dsn_list) {
  dsn_list_ = dsn_list;
  const auto host_count = dsn_list_.size();
  host_types_.resize(host_count, kNothing);

  for (size_t i = 0; i < host_count; ++i) {
    const auto hostname = FirstHostNameFromDsn(dsn_list_[i]);
    escaped_to_dsn_index_[EscapeHostName(hostname)] = i;
  }
}

void ClusterTopologyDiscovery::CreateConnections() {
  const auto host_count = dsn_list_.size();
  std::vector<engine::TaskWithResult<ConnectionPtr>> tasks;
  tasks.reserve(host_count);

  LOG_INFO() << "Creating connections to monitor cluster topology";
  for (auto&& dsn : dsn_list_) {
    tasks.push_back(Connect(dsn));
  }

  connections_.reserve(host_count);
  for (auto&& task : tasks) {
    connections_.push_back(task.Get());
  }
}

void ClusterTopologyDiscovery::StartPeriodicUpdates() {
  using ::utils::PeriodicTask;
  using Flags = ::utils::PeriodicTask::Flags;

  PeriodicTask::Settings settings(kInterval, {Flags::kNow});
  periodic_task_.Start(kPeriodicTaskName, settings,
                       [this] { CheckTopology(); });
}

engine::TaskWithResult<ConnectionPtr> ClusterTopologyDiscovery::Connect(
    std::string dsn) {
  return engine::Async([ this, dsn = std::move(dsn) ] {
    ConnectionPtr conn;
    try {
      conn = Connection::Connect(dsn, bg_task_processor_, kConnectionId);
    } catch (const ConnectionError&) {
      // No problem if can't connect right now - will try later
      // TODO re-create connection later
    }
    return conn;
  });
}

void ClusterTopologyDiscovery::CheckTopology() {
  LOG_INFO() << "Checking cluster topology";
  HostTypeList host_types;

  // TODO don't block if next call came soon, think of short path with skipping
  // close updates
  {
    std::lock_guard<engine::Mutex> lock(update_mutex_);

    host_types.resize(connections_.size(), kNothing);
    const auto master_index = FindMaster(host_types);
    FindSyncSlaves(host_types, master_index);
  }

  std::lock_guard<engine::Mutex> lock(hosts_mutex_);
  host_types_ = std::move(host_types);
}

size_t ClusterTopologyDiscovery::FindMaster(HostTypeList& host_types) const {
  const auto conn_count = connections_.size();
  std::vector<std::pair<size_t, engine::TaskWithResult<ClusterHostType>>> tasks;
  tasks.reserve(conn_count);
  for (size_t i = 0; i < conn_count; ++i) {
    auto conn = connections_[i].get();
    if (!conn) {
      // TODO start new connection
      continue;
    }

    auto task = engine::Async([conn] {
      // TODO exception handling here
      auto res = conn->Execute("select pg_is_in_recovery()");
      assert(res && "pg_is_in_recovery must return bool value");

      bool in_recovery = true;
      res.Front().To(in_recovery);
      return in_recovery ? ClusterHostType::kSlave : ClusterHostType::kMaster;
    });
    tasks.push_back(std::make_pair(i, std::move(task)));
  }

  size_t master_index = kInvalidIndex;
  for (auto && [ index, task ] : tasks) {
    const auto host_type = task.Get();
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
                                              size_t master_index) const {
  if (master_index == kInvalidIndex) {
    LOG_WARNING() << "No master hosts found";
    return;
  }

  // TODO take out sensitive info from dsn when logged
  LOG_INFO() << "Found master host: '" << dsn_list_[master_index] << '\'';
  auto task = engine::Async([ this, conn = connections_[master_index].get() ] {
    // TODO exception handling here
    auto res = conn->Execute("show synchronous_standby_names");
    if (!res) {
      return std::vector<size_t>{};
    }

    std::vector<size_t> sync_slave_indices;
    sync_slave_indices.reserve(res.Size());
    for (auto&& res_row : res) {
      const auto sync_slave_name = std::get<0>(res_row.As<std::string>());
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

  const auto& sync_slave_indices = task.Get();
  if (sync_slave_indices.empty()) {
    LOG_WARNING() << "No sync slave hosts found";
  }
  for (auto&& index : sync_slave_indices) {
    // TODO take out sensitive info from dsn when logged
    LOG_INFO() << "Found sync slave host: '" << dsn_list_[index] << '\'';
    if (index == master_index) {
      // TODO take out sensitive info from dsn when logged
      LOG_ERROR() << "Overwriting master type with sync slave type for host "
                  << dsn_list_[index];
    }
    host_types[index] = ClusterHostType::kSyncSlave;
  }
}

}  // namespace detail
}  // namespace postgres
}  // namespace storages
