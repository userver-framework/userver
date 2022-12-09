#pragma once

#include <memory>
#include <vector>

#include <userver/storages/mysql/cluster_host_type.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::infra {

class Pool;

class Topology final {
 public:
  Topology();
  ~Topology();

  Pool& SelectPool(ClusterHostType host_type);

 private:
  Pool& GetMaster() const;
  Pool& GetSlave() const;

  std::vector<std::shared_ptr<Pool>> pools_;
};

}  // namespace storages::mysql::infra

USERVER_NAMESPACE_END
