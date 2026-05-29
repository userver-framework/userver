#pragma once

#include <atomic>
#include <vector>

#include <storages/odbc/detail/topology/topology_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail::topology {

class FixedPrimary final : public TopologyBase {
public:
    FixedPrimary(const settings::ODBCClusterSettings& settings, clients::dns::Resolver* resolver);
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

}  // namespace storages::odbc::detail::topology

USERVER_NAMESPACE_END
