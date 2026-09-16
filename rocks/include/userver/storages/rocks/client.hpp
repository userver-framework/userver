#pragma once

/// @file userver/storages/rocks/client.hpp
/// @brief @copybrief storages::rocks::Client

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <rocksdb/db.h>

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/storages/rocks/query_context.hpp>
#include <userver/storages/rocks/snapshot.hpp>
#include <userver/storages/rocks/transaction.hpp>

namespace rocksdb {
class TransactionDB;
class OptimisticTransactionDB;
}  // namespace rocksdb

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

/// Transaction backend type, chosen at DB-open time.
/// Cannot be changed without recreating the database file.
enum class TransactionType {
    kNone,         ///< Plain DB, no transaction support.
    kPessimistic,  ///< Lock-based transactions (TransactionDB).
    kOptimistic,   ///< OCC transactions (OptimisticTransactionDB).
};

/**
 * @brief Client for working with RocksDB storage.
 *
 * This class provides an interface for interacting with the RocksDB database.
 * To use the class, you need to specify the database path when creating an
 * object.
 */
class Client final : public QueryContext {
public:
    /**
     * @brief Constructor of the Client class.
     *
     * @param db_path The path to the RocksDB database.
     * @param txn_type - transaction backend; determines how the DB is opened
     */
    Client(const std::string& db_path, TransactionType txn_type = TransactionType::kNone);

    /**
     * @brief Puts a record into the database.
     *
     * @param key The key of the record.
     * @param value The value of the record.
     */
    void Put(std::string_view key, std::string_view value) override;

    /**
     * @brief Retrieves the value of a record from the database by key.
     *
     * @param key The key of the record.
     */
    std::optional<std::string> Get(std::string_view key) override;

    /**
     * @brief Deletes a record from the database by key.
     *
     * @param key The key of the record to be deleted.
     */
    void Delete(std::string_view key) override;

    /// Creates a point-in-time snapshot of the database state.
    ///
    /// The returned Snapshot holds a shared reference to the underlying DB,
    /// so it remains valid even after this Client is destroyed.
    ///
    /// @throws storages::rocks::Exception if the DB does not support snapshots.
    Snapshot CreateSnapshot();

    /// Begins a new transaction.
    ///
    /// @throws storages::rocks::Exception if the Client was opened with
    /// TransactionType::kNone (transaction support not enabled).
    Transaction BeginTransaction();

    /// Returns a streaming cursor over all keys with the given @p prefix.
    /// If @p prefix is empty, scans all keys.
    ///
    /// The cursor holds an internal snapshot that fixes the read horizon at the
    /// moment Scan() is called — concurrent writes are not visible.
    Cursor Scan(std::string_view prefix = {});

private:
    void CheckStatus(rocksdb::Status status, std::string_view method_name);

    std::shared_ptr<rocksdb::DB> db_;
    engine::TaskProcessor& blocking_task_processor_;

    // Non-owning aliases into db_; at most one is non-null.
    rocksdb::TransactionDB* txn_db_{nullptr};
    rocksdb::OptimisticTransactionDB* opt_txn_db_{nullptr};
};

}  // namespace storages::rocks

USERVER_NAMESPACE_END
