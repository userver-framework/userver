#pragma once

#include <string>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::tracing {

inline const std::string kExecuteSpan{"mysql_execute"};
inline const std::string kTransactionSpan{"mysql_transaction"};
inline const std::string kQuerySpan{"mysql_query"};

inline const std::string kFetchScope{"mysql_fetch"};
inline const std::string kForEachScope{"mysql_foreach"};
inline const std::string kExecuteScope{"mysql_statement_execute"};

}  // namespace storages::mysql::impl::tracing

USERVER_NAMESPACE_END
