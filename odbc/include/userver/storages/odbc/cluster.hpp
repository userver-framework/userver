#pragma once

/// @file userver/storages/odbc/cluster.hpp
/// @brief @copybrief storages::odbc::Cluster

#include <chrono>
#include <optional>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <userver/storages/odbc/cluster_types.hpp>
#include <userver/storages/odbc/command_control.hpp>
#include <userver/storages/odbc/impl/parameter.hpp>
#include <userver/storages/odbc/query.hpp>
#include <userver/storages/odbc/result_set.hpp>
#include <userver/storages/odbc/settings.hpp>
#include <userver/storages/odbc/transaction.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

namespace detail {

class ClusterImpl;
using ClusterImplPtr = std::unique_ptr<ClusterImpl>;

}  // namespace detail

/// @brief ODBC cluster client: queries and transactions against pooled DSNs
class Cluster {
public:
    Cluster(const settings::ODBCClusterSettings& settings, clients::dns::Resolver* resolver);

    ~Cluster();

    /// @brief Execute a statement, binding every argument to an ODBC `?` placeholder.
    ///
    /// @warning Never interpolate untrusted values into @p query. Passing them as
    /// separate arguments ensures that they are sent to the ODBC driver as data.
    template <typename... Args>
    ResultSet Execute(ClusterHostTypeFlags flags, const Query& query, const Args&... args) {
        return Execute(flags, std::nullopt, query, args...);
    }

    /// @brief Execute a statement with per-operation timeout overrides.
    template <typename... Args>
    ResultSet Execute(
        ClusterHostTypeFlags flags,
        OptionalCommandControl command_control,
        const Query& query,
        const Args&... args
    ) {
        return DoExecute(command_control, flags, query, impl::MakeParameterList(args...));
    }

    Transaction Begin(ClusterHostTypeFlags flags);

    Transaction Begin(ClusterHostTypeFlags flags, OptionalCommandControl command_control);

    void WriteStatistics(utils::statistics::Writer& writer) const;

    /// @brief Set default command control (timeouts) from dynamic config
    void SetDefaultCommandControl(const CommandControl& cc);

    /// @brief Atomically replace cluster pools for future operations.
    /// Existing queries and transactions keep their old pools alive.
    void UpdateSettings(const settings::ODBCClusterSettings& settings);

    /// @cond
    void UpdateDsns(const std::vector<std::string>& dsns);
    void SetPoolSettingsOverride(std::optional<settings::PoolSettings> settings);
    /// @endcond

    /// @brief Get current default network timeout
    std::optional<std::chrono::milliseconds> GetDefaultNetworkTimeout() const;

    /// @brief Get current default statement timeout
    std::optional<std::chrono::milliseconds> GetDefaultStatementTimeout() const;

private:
    ResultSet DoExecute(
        OptionalCommandControl command_control,
        ClusterHostTypeFlags flags,
        const Query& query,
        const impl::ParameterList& parameters
    );

    detail::ClusterImplPtr impl_;
};

}  // namespace storages::odbc

USERVER_NAMESPACE_END
