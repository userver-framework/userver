#include <userver/storages/odbc/exception.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

RuntimeError::RuntimeError(std::string message, std::vector<DiagnosticRecord> diagnostics, bool invalid_handle)
    : Error(std::move(message)),
      diagnostics_(std::move(diagnostics)),
      invalid_handle_(invalid_handle)
{}

const std::vector<DiagnosticRecord>& RuntimeError::GetDiagnostics() const noexcept { return diagnostics_; }

bool RuntimeError::HasSqlStateClass(std::string_view sql_state_class) const noexcept {
    if (sql_state_class.size() != 2) {
        return false;
    }
    for (const auto& diagnostic : diagnostics_) {
        if (diagnostic.sql_state.size() >= 2 && diagnostic.sql_state.compare(0, 2, sql_state_class) == 0) {
            return true;
        }
    }
    return false;
}

bool RuntimeError::IsInvalidHandle() const noexcept { return invalid_handle_; }

FieldIndexOutOfBounds::FieldIndexOutOfBounds(std::size_t index)
    : ResultSetError(fmt::format("Field index {} is out of bounds", index))
{}

RowIndexOutOfBounds::RowIndexOutOfBounds(std::size_t index)
    : ResultSetError(fmt::format("Row index {} is out of bounds", index))
{}

}  // namespace storages::odbc

USERVER_NAMESPACE_END
