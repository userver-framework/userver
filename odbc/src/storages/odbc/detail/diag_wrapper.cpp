#include <fmt/format.h>
#include <storages/odbc/detail/diag_wrapper.hpp>

#include <algorithm>
#include <array>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

std::vector<DiagnosticRecord> GetSQLDiagnostics(SQLHANDLE handle, SQLSMALLINT type) {
    std::vector<DiagnosticRecord> diagnostics;

    for (SQLINTEGER i = 1;; ++i) {
        SQLINTEGER native = 0;
        std::array<SQLCHAR, SQL_SQLSTATE_SIZE + 1> state{};
        std::array<SQLCHAR, SQL_MAX_MESSAGE_LENGTH + 1> text{};
        SQLSMALLINT len = 0;

        const auto ret = SQLGetDiagRec(
            type,
            handle,
            static_cast<SQLSMALLINT>(i),
            state.data(),
            &native,
            text.data(),
            static_cast<SQLSMALLINT>(text.size()),
            &len
        );
        if (!SQL_SUCCEEDED(ret)) {
            break;
        }

        const auto message_length = std::min<std::size_t>(len > 0 ? static_cast<std::size_t>(len) : 0, text.size() - 1);
        diagnostics.push_back(DiagnosticRecord{
            .sql_state = std::string{reinterpret_cast<const char*>(state.data()), SQL_SQLSTATE_SIZE},
            .native_error = static_cast<int>(native),
            .message = std::string{reinterpret_cast<const char*>(text.data()), message_length},
        });
    }

    return diagnostics;
}

std::string FormatSQLDiagnostics(const std::vector<DiagnosticRecord>& diagnostics) {
    std::string result;
    for (const auto& diagnostic : diagnostics) {
        if (!result.empty()) {
            result += "; ";
        }
        result +=
            fmt::format("[{}] {} (native code {})", diagnostic.sql_state, diagnostic.message, diagnostic.native_error);
    }
    return result;
}

std::string GetSQLDiagString(SQLHANDLE handle, SQLSMALLINT type) {
    return FormatSQLDiagnostics(GetSQLDiagnostics(handle, type));
}

bool HasConnectionError(const std::vector<DiagnosticRecord>& diagnostics) noexcept {
    return std::any_of(diagnostics.begin(), diagnostics.end(), [](const DiagnosticRecord& diagnostic) {
        return diagnostic.sql_state.size() >= 2 && diagnostic.sql_state[0] == '0' && diagnostic.sql_state[1] == '8';
    });
}

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
