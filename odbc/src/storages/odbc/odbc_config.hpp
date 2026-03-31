#pragma once

#include <chrono>
#include <cstddef>
#include <optional>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

struct CommandControl {
    std::optional<std::chrono::milliseconds> network_timeout;
    std::optional<std::chrono::milliseconds> statement_timeout;
};

struct PoolSettingsDynamic {
    std::size_t min_pool_size{1};
    std::size_t max_pool_size{10};
};

struct Config final {
    static Config Parse(const dynamic_config::DocsMap& docs_map);

    CommandControl default_command_control;
    dynamic_config::ValueDict<PoolSettingsDynamic> pool_settings;
};

extern const dynamic_config::Key<Config> kConfig;

CommandControl Parse(const formats::json::Value& elem, formats::parse::To<CommandControl>);
PoolSettingsDynamic Parse(const formats::json::Value& elem, formats::parse::To<PoolSettingsDynamic>);

}  // namespace storages::odbc

USERVER_NAMESPACE_END
