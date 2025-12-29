#pragma once

#include <memory>
#include <string>

#include <storages/mongo/cdriver/wrappers.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/transaction.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl {

class PoolImpl;

struct TransactionData {
    TransactionData(Transaction::State state, cdriver::SessionPtr session)
        : state(state),
          session(std::move(session))
    {}

    Transaction::State state;
    cdriver::SessionPtr session;

    void EnsureActive() const;
};

/// @brief Implementation details for MongoDB transactions
class TransactionImpl {
public:
    explicit TransactionImpl(std::shared_ptr<PoolImpl> pool_impl);
    ~TransactionImpl();

    TransactionImpl(const TransactionImpl&) = delete;
    TransactionImpl& operator=(const TransactionImpl&) = delete;

    TransactionImpl(TransactionImpl&&) = delete;
    TransactionImpl& operator=(TransactionImpl&&) = delete;

    /// @brief Get a collection handle within transaction context
    Collection GetCollection(std::string name);

    /// @brief Commit the transaction
    void Commit();

    /// @brief Abort the transaction
    void Abort() noexcept;

    /// @brief Check if transaction is active
    bool IsActive() const noexcept;

    /// @brief Get current transaction state
    Transaction::State GetState() const noexcept;

private:
    void EnsureTransactionStarted();
    void EnsureActive() const;

    std::shared_ptr<PoolImpl> pool_impl_;
    std::shared_ptr<TransactionData> data_;

    // Database name for this transaction
    std::string database_name_;
};

}  // namespace storages::mongo::impl

USERVER_NAMESPACE_END
