#include <fmt/format.h>
#include <storages/odbc/detail/diag_wrapper.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

std::string GetSQLDiagString(SQLHANDLE handle, SQLSMALLINT type) {
    std::string result;

    for (SQLINTEGER i = 1;; ++i) {
        SQLINTEGER native = 0;
        SQLCHAR state[7];
        SQLCHAR text[SQL_MAX_MESSAGE_LENGTH];
        SQLSMALLINT len = 0;

        const auto ret = SQLGetDiagRec(type, handle, i, state, &native, text, sizeof(text), &len);
        if (!SQL_SUCCEEDED(ret)) {
            break;
        }

        result += fmt::format("{} (code {})", reinterpret_cast<char*>(&text[0]), native);
    }

    return result;
}

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
