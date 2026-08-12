#pragma once

#include <sql.h>
#include <sqlext.h>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

std::string GetSQLDiagString(SQLHANDLE handle, SQLSMALLINT type);

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
