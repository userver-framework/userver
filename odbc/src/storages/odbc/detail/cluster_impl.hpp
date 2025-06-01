#pragma once

#include <memory>
#include <string>
#include <vector>

#include <userver/storages/odbc/cluster_types.hpp>
#include <userver/storages/odbc/query.hpp>
#include <userver/storages/odbc/result_set.hpp>
#include <userver/storages/odbc/settings.hpp>

#include <storages/odbc/detail/pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

class ClusterImpl {
public:
    ClusterImpl(const settings::ODBCClusterSettings& settings);

    ~ClusterImpl() = default;

    ResultSet Execute(ClusterHostTypeFlags flags, const Query& query);

private:
    std::vector<std::string> dsns_;
    std::vector<std::shared_ptr<Pool>> pools_;
};
}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
