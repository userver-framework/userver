#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <userver/storages/odbc/bulk.hpp>
#include <userver/storages/odbc/cluster_types.hpp>
#include <userver/storages/odbc/command_control.hpp>
#include <userver/storages/odbc/cursor.hpp>
#include <userver/storages/odbc/impl/parameter.hpp>
#include <userver/storages/odbc/query.hpp>
#include <userver/storages/odbc/result_set.hpp>
#include <userver/storages/odbc/settings.hpp>
#include <userver/storages/odbc/transaction.hpp>

#include <storages/odbc/detail/bulk.hpp>
#include <storages/odbc/detail/command_control_store.hpp>
#include <storages/odbc/detail/pool.hpp>
#include <storages/odbc/detail/topology/topology_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

class ClusterImpl {
public:
    ClusterImpl(
        const settings::ODBCClusterSettings& settings,
        clients::dns::Resolver* resolver,
        engine::TaskProcessor& blocking_task_processor
    );

    ~ClusterImpl() = default;

    ResultSet Execute(
        ClusterHostTypeFlags flags,
        OptionalCommandControl command_control,
        const Query& query,
        const impl::ParameterList& parameters
    );

    Cursor ExecuteCursor(
        ClusterHostTypeFlags flags,
        OptionalCommandControl command_control,
        const Query& query,
        const impl::ParameterList& parameters
    );

    BulkResult ExecuteBulk(
        ClusterHostTypeFlags flags,
        OptionalCommandControl command_control,
        const Query& query,
        const impl::ParameterRows& rows,
        const BulkLayout& layout,
        std::size_t chunk_rows
    );

    Transaction Begin(ClusterHostTypeFlags flags);

    Transaction Begin(ClusterHostTypeFlags flags, OptionalCommandControl command_control);

    Transaction Begin(ClusterHostTypeFlags flags, const TransactionOptions& options);

    Transaction Begin(
        ClusterHostTypeFlags flags,
        const TransactionOptions& options,
        OptionalCommandControl command_control
    );

    void WriteStatistics(utils::statistics::Writer& writer) const;

    void SetDefaultCommandControl(const CommandControl& cc);
    void SetHandlersCommandControl(CommandControlByHandlerMap command_control);
    void SetQueriesCommandControl(CommandControlByQueryMap command_control);
    void SetStatementMetricsSettings(const settings::StatementMetricsSettings& settings);
    void SetPreparedStatementCacheSettings(const settings::PreparedStatementCacheSettings& settings);
    void SetPreparedStatementCacheSettingsOverride(std::optional<settings::PreparedStatementCacheSettings> settings);
    void ApplyDynamicCommandControls(
        CommandControl default_command_control,
        CommandControlByHandlerMap handlers_command_control,
        CommandControlByQueryMap queries_command_control
    );

    void UpdateSettings(const settings::ODBCClusterSettings& settings);
    void UpdateDsns(const std::vector<std::string>& dsns);
    void SetPoolSettingsOverride(std::optional<settings::PoolSettings> settings);

    std::optional<std::chrono::milliseconds> GetDefaultNetworkTimeout() const;

    std::optional<std::chrono::milliseconds> GetDefaultStatementTimeout() const;

private:
    static Pool& SelectPool(const topology::TopologyBase& topology, ClusterHostTypeFlags flags);

    ResultSet ExecuteImpl(
        engine::Deadline acquire_deadline,
        std::chrono::milliseconds statement_timeout,
        ClusterHostTypeFlags flags,
        const Query& query,
        const impl::ParameterList& parameters
    );

    BulkResult ExecuteBulkImpl(
        engine::Deadline acquire_deadline,
        std::chrono::milliseconds statement_timeout,
        ClusterHostTypeFlags flags,
        const Query& query,
        const impl::ParameterRows& rows,
        const BulkLayout& layout,
        std::size_t chunk_rows
    );

    CommandControl ResolveCommandControl(
        OptionalCommandControl command_control,
        std::optional<Query::NameView> query_name
    ) const;
    bool UpdateSettingsLocked(const settings::ODBCClusterSettings& settings);
    settings::ODBCClusterSettings MakeEffectiveSettingsLocked() const;
    void ApplyPreparedStatementCacheSettingsLocked();

    clients::dns::Resolver* resolver_;
    engine::TaskProcessor& blocking_task_processor_;
    std::shared_ptr<topology::TopologyBase> topology_;
    mutable engine::Mutex settings_mutex_;
    std::shared_ptr<const settings::ODBCClusterSettings> settings_;
    std::shared_ptr<const settings::ODBCClusterSettings> baseline_settings_;
    std::optional<settings::PoolSettings> pool_settings_override_;
    settings::StatementMetricsSettings statement_metrics_settings_{};
    settings::PreparedStatementCacheSettings prepared_statement_cache_settings_baseline_{};
    std::optional<settings::PreparedStatementCacheSettings> prepared_statement_cache_settings_override_;
    settings::PreparedStatementCacheSettings prepared_statement_cache_settings_effective_{};

    CommandControlStorePtr command_control_store_;
};

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
