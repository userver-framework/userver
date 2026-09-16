#include <userver/storages/odbc/cluster.hpp>

#include <storages/odbc/detail/bulk.hpp>
#include <storages/odbc/detail/cluster_impl.hpp>
#include <userver/storages/odbc/command_control.hpp>

#include <userver/engine/task/current_task.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

Cluster::Cluster(const settings::ODBCClusterSettings& settings, clients::dns::Resolver* resolver)
    : Cluster{settings, resolver, engine::current_task::GetBlockingTaskProcessor()}
{}

Cluster::Cluster(
    const settings::ODBCClusterSettings& settings,
    clients::dns::Resolver* resolver,
    engine::TaskProcessor& blocking_task_processor
)
    : impl_(std::make_unique<detail::ClusterImpl>(settings, resolver, blocking_task_processor))
{
    UASSERT(!settings.pools.empty());
}

Cluster::~Cluster() = default;

ResultSet Cluster::DoExecute(
    OptionalCommandControl command_control,
    ClusterHostTypeFlags flags,
    const Query& query,
    const impl::ParameterList& parameters
) {
    return impl_->Execute(flags, command_control, query, parameters);
}

ResultSet Cluster::Execute(ClusterHostTypeFlags flags, const Query& query, const ParameterStore& store) {
    return Execute(flags, std::nullopt, query, store);
}

ResultSet Cluster::Execute(
    ClusterHostTypeFlags flags,
    OptionalCommandControl command_control,
    const Query& query,
    const ParameterStore& store
) {
    return DoExecute(command_control, flags, query, store.GetParameters());
}

Cursor Cluster::DoExecuteCursor(
    OptionalCommandControl command_control,
    ClusterHostTypeFlags flags,
    const Query& query,
    const impl::ParameterList& parameters
) {
    return impl_->ExecuteCursor(flags, command_control, query, parameters);
}

Cursor Cluster::ExecuteCursor(ClusterHostTypeFlags flags, const Query& query, const ParameterStore& store) {
    return ExecuteCursor(flags, std::nullopt, query, store);
}

Cursor Cluster::ExecuteCursor(
    ClusterHostTypeFlags flags,
    OptionalCommandControl command_control,
    const Query& query,
    const ParameterStore& store
) {
    return DoExecuteCursor(command_control, flags, query, store.GetParameters());
}

BulkResult Cluster::ExecuteBulk(
    ClusterHostTypeFlags flags,
    const Query& query,
    const BulkParameterStore& rows,
    std::size_t chunk_rows
) {
    return ExecuteBulk(flags, std::nullopt, query, rows, chunk_rows);
}

BulkResult Cluster::ExecuteBulk(
    ClusterHostTypeFlags flags,
    OptionalCommandControl command_control,
    const Query& query,
    const BulkParameterStore& rows,
    std::size_t chunk_rows
) {
    if (chunk_rows == 0) {
        throw LogicError("ODBC bulk chunk size must be greater than zero");
    }
    if (rows.IsEmpty()) {
        return {};
    }
    const auto layout = detail::ValidateBulkRows(rows.GetRows());
    return DoExecuteBulk(command_control, flags, query, rows.GetRows(), layout, chunk_rows);
}

BulkResult Cluster::DoExecuteBulk(
    OptionalCommandControl command_control,
    ClusterHostTypeFlags flags,
    const Query& query,
    const impl::ParameterRows& rows,
    const detail::BulkLayout& layout,
    std::size_t chunk_rows
) {
    return impl_->ExecuteBulk(flags, command_control, query, rows, layout, chunk_rows);
}

Transaction Cluster::Begin(ClusterHostTypeFlags flags) { return impl_->Begin(flags); }

Transaction Cluster::Begin(ClusterHostTypeFlags flags, OptionalCommandControl command_control) {
    return impl_->Begin(flags, command_control);
}

Transaction Cluster::Begin(ClusterHostTypeFlags flags, const TransactionOptions& options) {
    return impl_->Begin(flags, options);
}

Transaction Cluster::Begin(
    ClusterHostTypeFlags flags,
    const TransactionOptions& options,
    OptionalCommandControl command_control
) {
    return impl_->Begin(flags, options, command_control);
}

void Cluster::WriteStatistics(utils::statistics::Writer& writer) const { impl_->WriteStatistics(writer); }

void Cluster::SetDefaultCommandControl(const CommandControl& cc) { impl_->SetDefaultCommandControl(cc); }

void Cluster::SetHandlersCommandControl(CommandControlByHandlerMap command_control) {
    impl_->SetHandlersCommandControl(std::move(command_control));
}

void Cluster::SetQueriesCommandControl(CommandControlByQueryMap command_control) {
    impl_->SetQueriesCommandControl(std::move(command_control));
}

void Cluster::SetStatementMetricsSettings(const settings::StatementMetricsSettings& settings) {
    impl_->SetStatementMetricsSettings(settings);
}

void Cluster::SetPreparedStatementCacheSettings(const settings::PreparedStatementCacheSettings& settings) {
    impl_->SetPreparedStatementCacheSettings(settings);
}

void Cluster::SetPreparedStatementCacheSettingsOverride(std::optional<settings::PreparedStatementCacheSettings> settings
) {
    impl_->SetPreparedStatementCacheSettingsOverride(settings);
}

void Cluster::UpdateSettings(const settings::ODBCClusterSettings& settings) { impl_->UpdateSettings(settings); }

void Cluster::UpdateDsns(const std::vector<std::string>& dsns) { impl_->UpdateDsns(dsns); }

void Cluster::SetPoolSettingsOverride(std::optional<settings::PoolSettings> settings) {
    impl_->SetPoolSettingsOverride(settings);
}

void Cluster::ApplyDynamicCommandControls(
    CommandControl default_command_control,
    CommandControlByHandlerMap handlers_command_control,
    CommandControlByQueryMap queries_command_control
) {
    impl_->ApplyDynamicCommandControls(
        std::move(default_command_control),
        std::move(handlers_command_control),
        std::move(queries_command_control)
    );
}

std::optional<std::chrono::milliseconds> Cluster::GetDefaultNetworkTimeout() const {
    return impl_->GetDefaultNetworkTimeout();
}

std::optional<std::chrono::milliseconds> Cluster::GetDefaultStatementTimeout() const {
    return impl_->GetDefaultStatementTimeout();
}

}  // namespace storages::odbc

USERVER_NAMESPACE_END
