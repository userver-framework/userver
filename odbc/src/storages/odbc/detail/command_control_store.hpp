#pragma once

#include <memory>
#include <optional>
#include <string_view>

#include <userver/rcu/rcu.hpp>
#include <userver/storages/odbc/command_control.hpp>
#include <userver/storages/odbc/query.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

struct CommandControlConfig final {
    CommandControl default_command_control;
    CommandControlByHandlerMap handlers_command_control;
    CommandControlByQueryMap queries_command_control;

    bool operator==(const CommandControlConfig&) const = default;
};

CommandControl MergeCommandControl(CommandControl lower, const CommandControl& higher);

CommandControl ResolveCommandControl(
    const CommandControlConfig& config,
    std::optional<std::string_view> handler_path,
    std::optional<std::string_view> handler_method,
    std::optional<Query::NameView> query_name,
    OptionalCommandControl explicit_command_control
);

class CommandControlStore final {
public:
    CommandControlStore();

    CommandControl Resolve(
        std::optional<std::string_view> handler_path,
        std::optional<std::string_view> handler_method,
        std::optional<Query::NameView> query_name,
        OptionalCommandControl explicit_command_control
    ) const;

    CommandControl ResolveTransactionStatement(
        CommandControl transaction_base,
        std::optional<Query::NameView> query_name,
        OptionalCommandControl explicit_command_control
    ) const;

    void SetDefault(CommandControl command_control);
    void SetHandlers(CommandControlByHandlerMap command_control);
    void SetQueries(CommandControlByQueryMap command_control);
    void Assign(CommandControlConfig config);

    CommandControl GetDefault() const;
    CommandControlConfig ReadCopy() const;

private:
    rcu::Variable<CommandControlConfig> config_;
};

using CommandControlStorePtr = std::shared_ptr<CommandControlStore>;

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
