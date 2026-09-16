#include <storages/odbc/detail/command_control_store.hpp>

#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

namespace {

const CommandControl* FindHandlerCommandControl(
    const CommandControlByHandlerMap& handlers,
    std::optional<std::string_view> path,
    std::optional<std::string_view> method
) {
    if (!path || !method) {
        return nullptr;
    }
    const auto* by_method = utils::FindOrNullptr(handlers, *path);
    return by_method ? utils::FindOrNullptr(*by_method, *method) : nullptr;
}

const CommandControl* FindQueryCommandControl(
    const CommandControlByQueryMap& queries,
    std::optional<Query::NameView> query_name
) {
    return query_name ? utils::FindOrNullptr(queries, *query_name) : nullptr;
}

}  // namespace

CommandControl MergeCommandControl(CommandControl lower, const CommandControl& higher) {
    if (higher.network_timeout) {
        lower.network_timeout = higher.network_timeout;
    }
    if (higher.statement_timeout) {
        lower.statement_timeout = higher.statement_timeout;
    }
    return lower;
}

CommandControl ResolveCommandControl(
    const CommandControlConfig& config,
    std::optional<std::string_view> handler_path,
    std::optional<std::string_view> handler_method,
    std::optional<Query::NameView> query_name,
    OptionalCommandControl explicit_command_control
) {
    auto resolved = config.default_command_control;
    if (const auto* handler = FindHandlerCommandControl(config.handlers_command_control, handler_path, handler_method))
    {
        resolved = MergeCommandControl(std::move(resolved), *handler);
    }
    if (const auto* query = FindQueryCommandControl(config.queries_command_control, query_name)) {
        resolved = MergeCommandControl(std::move(resolved), *query);
    }
    if (explicit_command_control) {
        resolved = MergeCommandControl(std::move(resolved), *explicit_command_control);
    }
    return resolved;
}

CommandControlStore::CommandControlStore() = default;

CommandControl CommandControlStore::Resolve(
    std::optional<std::string_view> handler_path,
    std::optional<std::string_view> handler_method,
    std::optional<Query::NameView> query_name,
    OptionalCommandControl explicit_command_control
) const {
    const auto config = config_.Read();
    return ResolveCommandControl(*config, handler_path, handler_method, query_name, explicit_command_control);
}

CommandControl CommandControlStore::ResolveTransactionStatement(
    CommandControl transaction_base,
    std::optional<Query::NameView> query_name,
    OptionalCommandControl explicit_command_control
) const {
    const auto config = config_.Read();
    if (const auto* query = FindQueryCommandControl(config->queries_command_control, query_name)) {
        transaction_base = MergeCommandControl(std::move(transaction_base), *query);
    }
    if (explicit_command_control) {
        transaction_base = MergeCommandControl(std::move(transaction_base), *explicit_command_control);
    }
    return transaction_base;
}

void CommandControlStore::SetDefault(CommandControl command_control) {
    auto config = config_.StartWrite();
    config->default_command_control = std::move(command_control);
    config.Commit();
}

void CommandControlStore::SetHandlers(CommandControlByHandlerMap command_control) {
    auto config = config_.StartWrite();
    config->handlers_command_control = std::move(command_control);
    config.Commit();
}

void CommandControlStore::SetQueries(CommandControlByQueryMap command_control) {
    auto config = config_.StartWrite();
    config->queries_command_control = std::move(command_control);
    config.Commit();
}

void CommandControlStore::Assign(CommandControlConfig config) { config_.Assign(std::move(config)); }

CommandControl CommandControlStore::GetDefault() const {
    const auto config = config_.Read();
    return config->default_command_control;
}

CommandControlConfig CommandControlStore::ReadCopy() const { return config_.ReadCopy(); }

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
