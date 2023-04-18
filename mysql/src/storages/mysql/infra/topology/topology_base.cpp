#include <storages/mysql/infra/topology/topology_base.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/wait_all_checked.hpp>

#include <storages/mysql/infra/pool.hpp>
#include <storages/mysql/infra/topology/fixed_primary.hpp>
#include <storages/mysql/infra/topology/standalone.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::infra::topology {

TopologyBase::TopologyBase(
    clients::dns::Resolver& resolver,
    const std::vector<settings::PoolSettings>& pools_settings) {
  const auto hosts_count = pools_settings.size();

  pools_.resize(hosts_count);
  std::vector<engine::TaskWithResult<void>> init_tasks;
  init_tasks.reserve(hosts_count);
  for (size_t i = 0; i < hosts_count; ++i) {
    auto& pool = pools_[i];
    init_tasks.emplace_back(engine::AsyncNoSpan(
        [&pool, &resolver, &settings = pools_settings[i]]() {
          pool = Pool::Create(resolver, settings);
        }));
  }

  engine::WaitAllChecked(init_tasks);
}

TopologyBase::~TopologyBase() = default;

std::unique_ptr<TopologyBase> TopologyBase::Create(
    clients::dns::Resolver& resolver,
    const std::vector<settings::PoolSettings>& pools_settings) {
  UASSERT(!pools_settings.empty());

  if (pools_settings.size() == 1) {
    return std::make_unique<infra::topology::Standalone>(resolver,
                                                         pools_settings);
  } else {
    return std::make_unique<infra::topology::FixedPrimary>(resolver,
                                                           pools_settings);
  }
}

Pool& TopologyBase::SelectPool(ClusterHostType host_type) const {
  switch (host_type) {
    case ClusterHostType::kPrimary:
      return GetPrimary();
    case ClusterHostType::kSecondary:
      return GetSecondary();
  }

  UINVARIANT(false, "Unknown host type");
}

void TopologyBase::WriteStatistics(utils::statistics::Writer& writer) const {
  for (const auto& pool : pools_) {
    pool->WriteStatistics(writer);
  }
}

}  // namespace storages::mysql::infra::topology

USERVER_NAMESPACE_END
