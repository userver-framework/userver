#pragma once

#include <string>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::impl::tracing {

inline const std::string kExecuteSpan{"odbc_execute"};
inline const std::string kTransactionSpan{"odbc_transaction"};
inline const std::string kQuerySpan{"odbc_query"};

}  // namespace storages::odbc::impl::tracing

USERVER_NAMESPACE_END
