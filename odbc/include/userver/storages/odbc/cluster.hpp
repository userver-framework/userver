#pragma once

/// @file userver/storages/odbc/cluster.hpp
/// @brief @copybrief storages::odbc::Cluster

#include <chrono>
#include <optional>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <userver/storages/odbc/cluster_types.hpp>
#include <userver/storages/odbc/query.hpp>
#include <userver/storages/odbc/result_set.hpp>
#include <userver/storages/odbc/settings.hpp>
#include <userver/storages/odbc/transaction.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

struct CommandControl;

namespace detail {

class ClusterImpl;
using ClusterImplPtr = std::unique_ptr<ClusterImpl>;

}  // namespace detail

/// @brief ODBC cluster client: queries and transactions against pooled DSNs
class Cluster {
public:
    Cluster(const settings::ODBCClusterSettings& settings, clients::dns::Resolver* resolver);

    ~Cluster();

    ResultSet Execute(ClusterHostTypeFlags flags, const Query& query);

    ResultSet Execute(engine::Deadline deadline, ClusterHostTypeFlags flags, const Query& query);

    Transaction Begin(ClusterHostTypeFlags flags);

    Transaction Begin(engine::Deadline deadline, ClusterHostTypeFlags flags);

    void WriteStatistics(utils::statistics::Writer& writer) const;

    /// @brief Set default command control (timeouts) from dynamic config
    void SetDefaultCommandControl(const CommandControl& cc);

    /// @brief Get current default network timeout
    std::optional<std::chrono::milliseconds> GetDefaultNetworkTimeout() const;

    /// @brief Get current default statement timeout
    std::optional<std::chrono::milliseconds> GetDefaultStatementTimeout() const;

private:
    detail::ClusterImplPtr impl_;
};

}  // namespace storages::odbc

USERVER_NAMESPACE_END
