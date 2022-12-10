#include <storages/mysql/infra/topology.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/wait_all_checked.hpp>

#include <storages/mysql/infra/pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::infra {

constexpr std::size_t kHostsCount = 1;

Topology::Topology() {
  const auto hosts_count = kHostsCount;

  pools_.resize(hosts_count);
  std::vector<engine::TaskWithResult<void>> init_tasks;
  init_tasks.reserve(hosts_count);
  for (size_t i = 0; i < hosts_count; ++i) {
    auto& pool = pools_[i];
    init_tasks.emplace_back(
        engine::AsyncNoSpan([&pool] { pool = Pool::Create(); }));
  }

  engine::WaitAllChecked(init_tasks);
}

Topology::~Topology() = default;

Pool& Topology::SelectPool(ClusterHostType host_type) const {
  switch (host_type) {
    case ClusterHostType::kMaster:
      return GetMaster();
    case ClusterHostType::kSlave:
      return GetSlave();
  }
}

Pool& Topology::GetMaster() const {
  // TODO
  return *pools_.back();
}

Pool& Topology::GetSlave() const {
  // TODO
  return *pools_.back();
}

}  // namespace storages::mysql::infra

USERVER_NAMESPACE_END
