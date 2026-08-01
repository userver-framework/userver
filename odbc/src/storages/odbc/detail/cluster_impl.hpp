#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <userver/storages/odbc/cluster_types.hpp>
#include <userver/storages/odbc/command_control.hpp>
#include <userver/storages/odbc/impl/parameter.hpp>
#include <userver/storages/odbc/query.hpp>
#include <userver/storages/odbc/result_set.hpp>
#include <userver/storages/odbc/settings.hpp>
#include <userver/storages/odbc/transaction.hpp>

#include <storages/odbc/detail/pool.hpp>
#include <storages/odbc/detail/topology/topology_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

class ClusterImpl {
public:
    ClusterImpl(const settings::ODBCClusterSettings& settings, clients::dns::Resolver* resolver);

    ~ClusterImpl() = default;

    ResultSet Execute(
        ClusterHostTypeFlags flags,
        OptionalCommandControl command_control,
        const Query& query,
        const impl::ParameterList& parameters
    );

    Transaction Begin(ClusterHostTypeFlags flags);

    Transaction Begin(ClusterHostTypeFlags flags, OptionalCommandControl command_control);

    void WriteStatistics(utils::statistics::Writer& writer) const;

    void SetDefaultCommandControl(const CommandControl& cc);

    void UpdateSettings(const settings::ODBCClusterSettings& settings);
    void UpdateDsns(const std::vector<std::string>& dsns);
    void SetPoolSettingsOverride(std::optional<settings::PoolSettings> settings);

    std::optional<std::chrono::milliseconds> GetDefaultNetworkTimeout() const;

    std::optional<std::chrono::milliseconds> GetDefaultStatementTimeout() const;

private:
    static Pool& SelectPool(const topology::TopologyBase& topology, ClusterHostTypeFlags flags);

    ResultSet ExecuteImpl(
        engine::Deadline acquire_deadline,
        engine::Deadline statement_deadline,
        ClusterHostTypeFlags flags,
        const Query& query,
        const impl::ParameterList& parameters
    );

    std::chrono::milliseconds ResolveNetworkTimeout(OptionalCommandControl command_control) const;
    std::chrono::milliseconds ResolveStatementTimeout(OptionalCommandControl command_control) const;
    bool UpdateSettingsLocked(const settings::ODBCClusterSettings& settings);
    settings::ODBCClusterSettings MakeEffectiveSettingsLocked() const;

    clients::dns::Resolver* resolver_;
    std::shared_ptr<topology::TopologyBase> topology_;
    mutable engine::Mutex settings_mutex_;
    std::shared_ptr<const settings::ODBCClusterSettings> settings_;
    std::shared_ptr<const settings::ODBCClusterSettings> baseline_settings_;
    std::optional<settings::PoolSettings> pool_settings_override_;

    // Dynamic config: command control (timeouts)
    std::atomic<std::chrono::milliseconds> default_network_timeout_ms_{std::chrono::milliseconds::zero()};
    std::atomic<std::chrono::milliseconds> default_statement_timeout_ms_{std::chrono::milliseconds::zero()};
    std::atomic<bool> has_network_timeout_{false};
    std::atomic<bool> has_statement_timeout_{false};
};

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
