#pragma once

/// @file userver/storages/odbc/tests/utils.hpp
/// @brief Utilities for testing logic working with ODBC.

#include <userver/storages/odbc.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::tests {

constexpr auto kDSN =
    "DRIVER={PostgreSQL Unicode};"
    "SERVER=localhost;"
    "PORT=15433;"
    "DATABASE=postgres;"
    "UID=testsuite;"
    "PWD=password;";

auto kHostSettings = storages::odbc::settings::HostSettings{kDSN, {}};
auto kSettings = storages::odbc::settings::ODBCClusterSettings{{kHostSettings}};
auto kMultiDSNSettings = storages::odbc::settings::ODBCClusterSettings{{kHostSettings, kHostSettings}};
} // namespace storages::odbc::tests

USERVER_NAMESPACE_END