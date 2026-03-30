#pragma once

#include <memory>
#include <vector>

#include <userver/storages/odbc/cluster_types.hpp>
#include <userver/storages/odbc/settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

class Pool;

namespace topology {

class TopologyBase {
public:
    virtual ~TopologyBase();

    static std::unique_ptr<TopologyBase> Create(const settings::ODBCClusterSettings& settings);

    Pool& SelectPool(ClusterHostType host_type) const;

protected:
    explicit TopologyBase(const settings::ODBCClusterSettings& settings);

    virtual Pool& GetPrimary() const = 0;
    virtual Pool& GetSecondary() const = 0;

    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    std::vector<std::shared_ptr<Pool>> pools_;
};

}  // namespace topology

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END

