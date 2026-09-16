#pragma once

#include <optional>
#include <string_view>

#include <sql.h>
#include <sqlext.h>

#include <userver/storages/odbc/transaction_options.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

SQLUINTEGER ToOdbcIsolation(IsolationLevel isolation) noexcept;

std::string_view ToStringView(IsolationLevel isolation) noexcept;

bool IsIsolationSupported(std::optional<SQLUINTEGER> supported_mask, IsolationLevel isolation) noexcept;

bool IsExactConnectionAttributeValue(SQLUINTEGER requested, SQLUINTEGER actual) noexcept;

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
