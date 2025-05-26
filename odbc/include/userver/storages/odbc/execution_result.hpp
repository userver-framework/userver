#pragma once

/// @file userver/storages/odbc/execution_result.hpp

#include <cstdint>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

/// @brief Metadata for an execution of a statement that doesn't expect a result
/// set (INSERT, UPDATE, DELETE).
struct ExecutionResult final {
    /// Amount of rows that statement affected. Consult ODBC docs for better
    /// understanding.
    std::uint64_t rows_affected{};

    /// LastInsertId, if any, or zero.
    std::uint64_t last_insert_id{};
};

}  // namespace storages::odbc

USERVER_NAMESPACE_END
