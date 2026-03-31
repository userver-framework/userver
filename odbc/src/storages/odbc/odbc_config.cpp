#include "odbc_config.hpp"

#include <fmt/format.h>

#include <userver/formats/common/items.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/odbc/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

CommandControl Parse(const formats::json::Value& elem, formats::parse::To<CommandControl>) {
    CommandControl result{};
    for (const auto& [name, val] : formats::common::Items(elem)) {
        if (name == "network_timeout_ms") {
            const auto ms = std::chrono::milliseconds{val.As<std::int64_t>()};
            result.network_timeout = ms;
            if (ms.count() <= 0) {
                throw Error{
                    "Invalid network_timeout_ms `" + std::to_string(ms.count()) +
                    "` in ODBC CommandControl. The timeout must be greater than 0."
                };
            }
        } else if (name == "statement_timeout_ms") {
            const auto ms = std::chrono::milliseconds{val.As<std::int64_t>()};
            result.statement_timeout = ms;
            if (ms.count() <= 0) {
                throw Error{
                    "Invalid statement_timeout_ms `" + std::to_string(ms.count()) +
                    "` in ODBC CommandControl. The timeout must be greater than 0."
                };
            }
        } else {
            LOG_WARNING() << "Unknown parameter " << name << " in ODBC config";
        }
    }
    return result;
}

PoolSettingsDynamic Parse(const formats::json::Value& elem, formats::parse::To<PoolSettingsDynamic>) {
    PoolSettingsDynamic result{};

    result.min_pool_size = elem["min_pool_size"].As<std::size_t>(result.min_pool_size);
    result.max_pool_size = elem["max_pool_size"].As<std::size_t>(result.max_pool_size);

    if (result.max_pool_size == 0) {
        throw Error{"max_pool_size must be greater than 0"};
    }
    if (result.max_pool_size < result.min_pool_size) {
        throw Error{fmt::format(
            "max_pool_size cannot be less than min_pool_size. max_pool_size={}, min_pool_size={}",
            result.max_pool_size,
            result.min_pool_size
        )};
    }

    return result;
}

Config Config::Parse(const dynamic_config::DocsMap& docs_map) {
    return Config{
        /*default_command_control=*/docs_map.Get("ODBC_DEFAULT_COMMAND_CONTROL").As<CommandControl>(),
        /*pool_settings=*/
        docs_map.Get("ODBC_CONNECTION_POOL_SETTINGS").As<dynamic_config::ValueDict<PoolSettingsDynamic>>(),
    };
}

using JsonString = dynamic_config::DefaultAsJsonString;

const dynamic_config::Key<Config> kConfig{
    Config::Parse,
    {
        {"ODBC_DEFAULT_COMMAND_CONTROL", JsonString{"{}"}},
        {"ODBC_CONNECTION_POOL_SETTINGS", JsonString{"{}"}},
    },
};

}  // namespace storages::odbc

USERVER_NAMESPACE_END
