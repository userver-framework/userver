#pragma once

#include <memory>
#include <vector>

#include <storages/mysql/settings/host_settings.hpp>
#include <userver/storages/mysql/cluster_host_type.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::infra {

class Pool;

namespace topology {

class TopologyBase {
 public:
  virtual ~TopologyBase();

  Pool& SelectPool(ClusterHostType host_type) const;

  static std::unique_ptr<TopologyBase> Create(
      std::vector<settings::HostSettings>&& hosts_settings);

 protected:
  explicit TopologyBase(std::vector<settings::HostSettings> hosts_settings);

  virtual Pool& GetMaster() const = 0;
  virtual Pool& GetSlave() const = 0;

  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::vector<std::shared_ptr<Pool>> pools_;
};

}  // namespace topology
}  // namespace storages::mysql::infra

USERVER_NAMESPACE_END
