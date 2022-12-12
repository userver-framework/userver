#include <storages/mysql/infra/topology/topology_base.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/wait_all_checked.hpp>

#include <storages/mysql/infra/pool.hpp>
#include <storages/mysql/infra/topology/standalone.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::infra::topology {

TopologyBase::TopologyBase(std::vector<settings::HostSettings> hosts_settings) {
  const auto hosts_count = hosts_settings.size();

  pools_.resize(hosts_count);
  std::vector<engine::TaskWithResult<void>> init_tasks;
  init_tasks.reserve(hosts_count);
  for (size_t i = 0; i < hosts_count; ++i) {
    auto& pool = pools_[i];
    init_tasks.emplace_back(engine::AsyncNoSpan(
        [&pool, settings = std::move(hosts_settings[i])]() mutable {
          pool = Pool::Create(std::move(settings));
        }));
  }

  engine::WaitAllChecked(init_tasks);
}

TopologyBase::~TopologyBase() = default;

std::unique_ptr<TopologyBase> TopologyBase::Create(
    std::vector<settings::HostSettings>&& hosts_settings) {
  UASSERT(!hosts_settings.empty());

  if (hosts_settings.size() == 1) {
    return std::make_unique<infra::topology::Standalone>(
        std::move(hosts_settings));
  }

  UINVARIANT(false, "Multihost topology is not yet implemented");
}

Pool& TopologyBase::SelectPool(ClusterHostType host_type) const {
  switch (host_type) {
    case ClusterHostType::kMaster:
      return GetMaster();
    case ClusterHostType::kSlave:
      return GetSlave();
  }
}

}  // namespace storages::mysql::infra::topology

USERVER_NAMESPACE_END
