#pragma once

#include <memory>
#include <vector>

#include <storages/mysql/settings/host_settings.hpp>
#include <userver/storages/mysql/cluster_host_type.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::infra {

class Pool;

class Topology final {
 public:
  Topology(std::vector<settings::HostSettings> hosts_settings);
  ~Topology();

  Pool& SelectPool(ClusterHostType host_type) const;

 private:
  Pool& GetMaster() const;
  Pool& GetSlave() const;

  std::vector<std::shared_ptr<Pool>> pools_;
};

}  // namespace storages::mysql::infra

USERVER_NAMESPACE_END
