#pragma once

#include <storages/mysql/infra/topology/topology_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::infra::topology {

class Standalone final : public TopologyBase {
 public:
  Standalone(clients::dns::Resolver& resolver,
             const std::vector<settings::PoolSettings>& pools_settings);
  ~Standalone() final;

 private:
  Pool& GetMaster() const final;
  Pool& GetSlave() const final;

  Pool& InitializePool() const;

  Pool& pool_;
};

}  // namespace storages::mysql::infra::topology

USERVER_NAMESPACE_END
