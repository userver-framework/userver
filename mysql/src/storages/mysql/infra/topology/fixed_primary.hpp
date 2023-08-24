#pragma once

#include <atomic>

#include <storages/mysql/infra/topology/topology_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::infra::topology {

class FixedPrimary final : public TopologyBase {
 public:
  FixedPrimary(clients::dns::Resolver& resolver,
               const std::vector<settings::PoolSettings>& settings);
  ~FixedPrimary() final;

 private:
  Pool& GetPrimary() const final;
  Pool& GetSecondary() const final;

  Pool& InitializePrimaryPoolReference();
  std::vector<Pool*> InitializeSecondariesVector();

  Pool& primary_;
  std::vector<Pool*> secondaries_;

  mutable std::atomic<std::size_t> secondary_index_{0};
};

}  // namespace storages::mysql::infra::topology

USERVER_NAMESPACE_END
