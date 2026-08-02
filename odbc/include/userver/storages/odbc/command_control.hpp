#pragma once

/// @file userver/storages/odbc/command_control.hpp
/// @brief Per-operation timeout settings for the ODBC driver.

#include <chrono>
#include <optional>
#include <string>

#include <userver/utils/impl/transparent_hash.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

struct CommandControl final {
    /// Overall operation budget used for pool waiting and connection login and
    /// as an upper bound for statement/transaction deadlines. Blocking ODBC
    /// calls are ultimately subject to the timeout capabilities and whole-
    /// second resolution of the selected ODBC driver.
    std::optional<std::chrono::milliseconds> network_timeout;

    /// Timeout for statement execution. ODBC drivers accept this timeout in
    /// whole seconds, so the value is rounded up when passed to a driver.
    std::optional<std::chrono::milliseconds> statement_timeout;

    bool operator==(const CommandControl&) const = default;
};

using OptionalCommandControl = std::optional<CommandControl>;

/// Command controls keyed by an HTTP method.
using CommandControlByMethodMap = utils::impl::TransparentMap<std::string, CommandControl>;

/// Command controls keyed first by handler path, then by HTTP method.
using CommandControlByHandlerMap = utils::impl::TransparentMap<std::string, CommandControlByMethodMap>;

/// Command controls keyed by storages::odbc::Query name.
using CommandControlByQueryMap = utils::impl::TransparentMap<std::string, CommandControl>;

}  // namespace storages::odbc

USERVER_NAMESPACE_END
