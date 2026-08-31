#pragma once

/// @file userver/storages/rocks/transaction.hpp
/// @brief @copybrief storages::rocks::Transaction

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/storages/rocks/query_context.hpp>

namespace rocksdb {
class DB;
class Transaction;
}  // namespace rocksdb

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

class Client;

/// @brief Represents an in-flight RocksDB transaction.
///
/// Writes are buffered and applied atomically on Commit().
/// Get() reads include uncommitted writes made by this transaction.
///
/// The transaction type (pessimistic or optimistic) is determined by how
/// the Client was opened (transaction-type config field).
///
/// Pessimistic: GetForUpdate() acquires a lock; Commit() always succeeds
/// if locks are held.
///
/// Optimistic: GetForUpdate() only tracks the key; Commit() may throw
/// WriteConflictException if a concurrent write was detected.
///
/// The transaction is automatically rolled back on destruction if neither
/// Commit() nor Rollback() was called.
class Transaction final : public QueryContext {
public:
    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
    Transaction(Transaction&&) noexcept;
    Transaction& operator=(Transaction&&) noexcept;
    ~Transaction();

    /// Write key=value, buffered until Commit().
    void Put(std::string_view key, std::string_view value) override;

    /// Delete key, buffered until Commit().
    void Delete(std::string_view key) override;

    /// Read key, including uncommitted writes made by this transaction.
    /// Returns nullopt if absent.
    std::optional<std::string> Get(std::string_view key) override;

    /// Read key and register it for conflict detection.
    ///
    /// Pessimistic: acquires a lock; throws LockTimeoutException on timeout.
    /// Optimistic: tracks the key for validation at Commit() time; never blocks.
    ///
    /// Returns nullopt if absent.
    std::optional<std::string> GetForUpdate(std::string_view key);

    /// Atomically applies all buffered writes.
    ///
    /// Pessimistic: succeeds unless the transaction has expired.
    /// Optimistic: throws WriteConflictException if a conflict was detected.
    void Commit();

    /// Discards all buffered writes and releases any held locks.
    void Rollback();

    /// Records a save point. RollbackToSavePoint() undoes writes since here.
    void SetSavePoint();

    /// Undoes all writes since the most recent SetSavePoint() and removes it.
    /// Throws if no save point exists.
    void RollbackToSavePoint();

private:
    friend class Client;
    Transaction(std::shared_ptr<rocksdb::DB> db, engine::TaskProcessor& tp, std::unique_ptr<rocksdb::Transaction> txn);

    std::shared_ptr<rocksdb::DB> db_;
    engine::TaskProcessor* tp_;
    std::unique_ptr<rocksdb::Transaction> txn_;
    bool done_{false};
};

}  // namespace storages::rocks

USERVER_NAMESPACE_END
