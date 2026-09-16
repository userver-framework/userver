#pragma once

/// @file userver/storages/rocks/query_context.hpp
/// @brief @copybrief storages::rocks::QueryContext

#include <optional>
#include <string>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

/// @brief Common interface for executing RocksDB operations, implemented by
/// both Client and Transaction.
///
/// Allows query logic to be written once and reused regardless of whether
/// the caller operates directly on the database or within a transaction.
///
/// @note Writes to a Client take effect immediately. Writes to a Transaction
/// are buffered and applied atomically on Transaction::Commit().
/// Transaction::Get() also reads the transaction's own uncommitted writes.
class QueryContext {
public:
    virtual ~QueryContext() = default;

    virtual std::optional<std::string> Get(std::string_view key) = 0;
    virtual void Put(std::string_view key, std::string_view value) = 0;
    virtual void Delete(std::string_view key) = 0;
};

}  // namespace storages::rocks

USERVER_NAMESPACE_END
