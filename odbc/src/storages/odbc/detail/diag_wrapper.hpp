#pragma once

#include <sql.h>
#include <sqlext.h>
#include <string>
#include <string_view>
#include <vector>

#include <userver/storages/odbc/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

std::vector<DiagnosticRecord> GetSQLDiagnostics(SQLHANDLE handle, SQLSMALLINT type);

std::string FormatSQLDiagnostics(const std::vector<DiagnosticRecord>& diagnostics);

std::string GetSQLDiagString(SQLHANDLE handle, SQLSMALLINT type);

bool HasConnectionError(const std::vector<DiagnosticRecord>& diagnostics) noexcept;

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
