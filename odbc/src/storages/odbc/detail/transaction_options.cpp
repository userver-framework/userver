#include <storages/odbc/detail/transaction_options.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

SQLUINTEGER ToOdbcIsolation(IsolationLevel isolation) noexcept {
    switch (isolation) {
        case IsolationLevel::kReadUncommitted:
            return SQL_TXN_READ_UNCOMMITTED;
        case IsolationLevel::kReadCommitted:
            return SQL_TXN_READ_COMMITTED;
        case IsolationLevel::kRepeatableRead:
            return SQL_TXN_REPEATABLE_READ;
        case IsolationLevel::kSerializable:
            return SQL_TXN_SERIALIZABLE;
    }
    UINVARIANT(false, "Unknown ODBC isolation level");
}

std::string_view ToStringView(IsolationLevel isolation) noexcept {
    switch (isolation) {
        case IsolationLevel::kReadUncommitted:
            return "READ UNCOMMITTED";
        case IsolationLevel::kReadCommitted:
            return "READ COMMITTED";
        case IsolationLevel::kRepeatableRead:
            return "REPEATABLE READ";
        case IsolationLevel::kSerializable:
            return "SERIALIZABLE";
    }
    UINVARIANT(false, "Unknown ODBC isolation level");
}

bool IsIsolationSupported(std::optional<SQLUINTEGER> supported_mask, IsolationLevel isolation) noexcept {
    return supported_mask && (*supported_mask & ToOdbcIsolation(isolation)) != 0;
}

bool IsExactConnectionAttributeValue(SQLUINTEGER requested, SQLUINTEGER actual) noexcept { return requested == actual; }

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
