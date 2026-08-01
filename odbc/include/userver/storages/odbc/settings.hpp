#pragma once

/// @file userver/storages/odbc/settings.hpp
/// @brief ODBC cluster static configuration (DSN pools)

#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::settings {

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
