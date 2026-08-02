#pragma once

/// @file userver/storages/odbc/settings.hpp
/// @brief ODBC cluster static configuration (DSN pools)

#include <cstddef>
#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::settings {

/// @brief Named ODBC query metrics options.
struct StatementMetricsSettings final {
    /// Maximum number of named query-name entries retained by each ODBC pool.
    /// Each entry exports three metric series. A value of 0 disables named
    /// query metrics.
    std::size_t max_statements{0};

    bool operator==(const StatementMetricsSettings&) const = default;
};

struct PoolSettings final {
    std::size_t min_size{5};
    std::size_t max_size{10};

    bool operator==(const PoolSettings&) const = default;
};

struct HostSettings final {
    const std::string dsn;
    PoolSettings pool;

    bool operator==(const HostSettings&) const = default;
};

struct ODBCClusterSettings final {
    std::vector<HostSettings> pools;

    bool operator==(const ODBCClusterSettings&) const = default;
};

}  // namespace storages::odbc::settings

USERVER_NAMESPACE_END
