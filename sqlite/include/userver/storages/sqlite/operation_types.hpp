#pragma once

/// @file userver/storages/sqlite/operations_types.hpp
/// @brief Operation properties

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

enum class OperationType {
    /// Execute a query on read only pool
    kReadOnly,
    /// Execute a query on a read write pool
    kReadWrite,
};

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
