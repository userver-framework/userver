#pragma once

#include <memory>
#include <string>
#include <vector>

#include <userver/storages/odbc/cluster_types.hpp>
#include <userver/storages/odbc/query.hpp>
#include <userver/storages/odbc/result_set.hpp>

#include <storages/odbc/detail/connection.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

class ClusterImpl {
public:
    ClusterImpl(const std::vector<std::string>& dsns);

    ~ClusterImpl() = default;

    ResultSet Execute(ClusterHostTypeFlags flags, const Query& query);

private:
    std::vector<std::string> dsns_;
    std::vector<std::unique_ptr<Connection>> connections_;
};
}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
