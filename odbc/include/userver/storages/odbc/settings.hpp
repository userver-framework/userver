#pragma once

#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::settings {

struct PoolSettings final {
    std::size_t minSize{5};
    std::size_t maxSize{10};
};

struct HostSettings final {
    const std::string dsn;
    PoolSettings pool;
};

struct ODBCClusterSettings final {
    std::vector<HostSettings> pools;
};

}  // namespace storages::odbc::settings

USERVER_NAMESPACE_END
