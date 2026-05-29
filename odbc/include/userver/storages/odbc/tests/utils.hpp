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

inline const auto kHostSettings = storages::odbc::settings::HostSettings{kDSN, {}};
inline const auto kSettings = storages::odbc::settings::ODBCClusterSettings{{kHostSettings}};
inline const auto kMultiDSNSettings = storages::odbc::settings::ODBCClusterSettings{{kHostSettings, kHostSettings}};

inline storages::odbc::Cluster MakeCluster(const storages::odbc::settings::ODBCClusterSettings& settings = kSettings) {
    return storages::odbc::Cluster{settings, nullptr};
}

}  // namespace storages::odbc::tests

USERVER_NAMESPACE_END
