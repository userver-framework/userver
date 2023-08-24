#pragma once

#include <string>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::impl::scopes {

inline const std::string kConnect = "clickhouse_connect";
inline const std::string kExec = "clickhouse_exec";

inline const std::string kQuery = "clickhouse_query";
inline const std::string kInsert = "clickhouse_insert";

}  // namespace storages::clickhouse::impl::scopes

USERVER_NAMESPACE_END
