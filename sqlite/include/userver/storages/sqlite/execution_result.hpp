#pragma once

/// @file userver/storages/sqlite/execution_result.hpp

#include <cstdint>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

/// @brief Metadata for an execution of a statement that doesn't expect a result
/// set (INSERT, UPDATE, DELETE).
struct ExecutionResult final {
    /// Amount of rows that statement affected. Consult SQLite docs for better
    /// understanding.
    std::int64_t rows_affected{};

    /// LastInsertId, if any, or zero.
    std::int64_t last_insert_id{};
};

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
