#pragma once

#include <memory>
#include <vector>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <userver/storages/mysql/cluster_host_type.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace settings {
struct PoolSettings;
}

namespace infra {

class Pool;

namespace topology {

class TopologyBase {
 public:
  virtual ~TopologyBase();

  static std::unique_ptr<TopologyBase> Create(
      clients::dns::Resolver& resolver,
      const std::vector<settings::PoolSettings>& pools_settings);

  Pool& SelectPool(ClusterHostType host_type) const;

  void WriteStatistics(utils::statistics::Writer& writer) const;

 protected:
  explicit TopologyBase(
      clients::dns::Resolver& resolver,
      const std::vector<settings::PoolSettings>& pools_settings);

  virtual Pool& GetPrimary() const = 0;
  virtual Pool& GetSecondary() const = 0;

  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::vector<std::shared_ptr<Pool>> pools_;
};
}  // namespace topology
}  // namespace infra
}  // namespace storages::mysql

USERVER_NAMESPACE_END
