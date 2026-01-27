#pragma once

/// @file userver/storages/mongo/transaction.hpp
/// @brief @copybrief storages::mongo::Transaction

#include <memory>
#include <string>

#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/options.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

namespace impl {
class TransactionImpl;
}  // namespace impl

/// @brief MongoDB transaction handle that provides ACID guarantees.
///
/// Transactions group multiple operations within logical units of work
/// and provide ACID guarantees. All operations in a transaction either
/// succeed completely or fail completely.
///
/// ## Usage example:
///
/// @snippet mongo/src/storages/mongo/transaction_mongotest.cpp transaction
///
/// @note Transactions require MongoDB replica sets or sharded clusters.
/// @note Operations within a transaction are automatically retried according
///       to MongoDB transaction retry semantics.
class Transaction {
public:
    /// @brief Creates invalid transaction
    Transaction();

    /// @cond
    // For internal use only.
    explicit Transaction(std::unique_ptr<impl::TransactionImpl>);
    /// @endcond

    /// @brief Move constructor
    Transaction(Transaction&&) noexcept;

    /// @brief Move assignment operator
    Transaction& operator=(Transaction&&) noexcept;

    /// @brief Destructor. Aborts transaction if not committed.
    ~Transaction();

    /// @brief Get a collection handle within the transaction context
    /// @param name collection name
    /// @return Collection handle that operates within this transaction
    Collection GetCollection(std::string name);

    /// @brief Commit the transaction
    ///
    /// Makes all changes within the transaction permanent and visible
    /// to other operations. If the transaction fails to commit,
    /// an exception will be thrown.
    ///
    /// @throws MongoException on commit failure
    void Commit();

    /// @brief Abort the transaction
    ///
    /// Discards all changes made within the transaction.
    /// This method is idempotent and safe to call multiple times.
    void Abort() noexcept;

    /// @brief Check if transaction is active
    /// @return true if transaction is active (not committed or aborted)
    bool IsActive() const noexcept;

    /// @brief Get the transaction state
    enum class State {
        kNone,        ///< Transaction not started
        kInProgress,  ///< Transaction is active
        kCommitted,   ///< Transaction has been committed
        kAborted      ///< Transaction has been aborted
    };

    /// @brief Get current transaction state
    State GetState() const noexcept;

private:
    std::unique_ptr<impl::TransactionImpl> impl_;
};

}  // namespace storages::mongo

USERVER_NAMESPACE_END
