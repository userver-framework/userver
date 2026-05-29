#pragma once

#include <memory>
#include <string>
#include <vector>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <userver/storages/odbc/cluster_types.hpp>
#include <userver/storages/odbc/settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

class Pool;

namespace topology {

class TopologyBase {
public:
    virtual ~TopologyBase();

    static std::unique_ptr<TopologyBase> Create(
        const settings::ODBCClusterSettings& settings,
        clients::dns::Resolver* resolver
    );

    Pool& SelectPool(ClusterHostType host_type) const;

    void WriteStatistics(utils::statistics::Writer& writer) const;

protected:
    TopologyBase(const settings::ODBCClusterSettings& settings, clients::dns::Resolver* resolver);

    virtual Pool& GetPrimary() const = 0;
    virtual Pool& GetSecondary() const = 0;

    const std::vector<std::shared_ptr<Pool>>& GetPools() const { return pools_; }

private:
    std::vector<std::shared_ptr<Pool>> pools_;
};

}  // namespace topology

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
