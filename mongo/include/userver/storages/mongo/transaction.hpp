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
/// A transaction pins exactly one connection from the pool starting from the
/// first call to storages::mongo::Transaction::GetCollection and holds it
/// until the transaction completes, @ref storages::mongo::Transaction::Abort
/// is called, or the transaction is destroyed.
/// An unknown commit result retains the connection for another commit attempt.
///
/// ## Usage example:
///
/// @snippet mongo/src/storages/mongo/transaction_mongotest.cpp transaction
///
/// @note Transactions require MongoDB replica sets or sharded clusters.
/// @note A transient transaction error requires retrying the whole transaction.
///       An unknown commit result requires retrying commit on the same transaction.
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

    /// @brief Aborts an active transaction or releases an unresolved commit session.
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
    /// @throws storages::mongo::TransactionException with
    /// @ref storages::mongo::TransactionException::Type::kUnknownCommitResult
    /// if the outcome is unknown. Retains the session for another commit attempt;
    /// collection operations are forbidden until the outcome is resolved.
    void Commit();

    /// @brief Abort the transaction
    ///
    /// Discards all changes made within the transaction.
    /// This method is idempotent and safe to call multiple times.
    /// If the commit result is unknown, releases the session without reporting
    /// a rollback or changing @ref storages::mongo::Transaction::State::kCommitUnknown.
    /// Commit cannot be retried after the session has been released.
    void Abort() noexcept;

    /// @brief Check whether the transaction is in progress.
    /// @return true only in @ref storages::mongo::Transaction::State::kInProgress.
    /// @note Returns false when the commit outcome is unknown.
    /// Commit may still be retried while the session is retained.
    bool IsActive() const noexcept;

    /// @brief Get the transaction state
    enum class State {
        kNone,          ///< Transaction not started
        kInProgress,    ///< Transaction is active
        kCommitted,     ///< Transaction has been committed
        kAborted,       ///< Transaction has been aborted
        kCommitUnknown  ///< Commit outcome is unknown; collection operations are forbidden
    };

    /// @brief Get current transaction state
    State GetState() const noexcept;

private:
    std::unique_ptr<impl::TransactionImpl> impl_;
};

}  // namespace storages::mongo

USERVER_NAMESPACE_END
