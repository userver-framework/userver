#pragma once

/// @file userver/storages/odbc/transaction.hpp

#include <userver/engine/deadline.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/trx_tracker.hpp>

#include <userver/storages/odbc/result_set.hpp>
#include <userver/storages/odbc/query.hpp>

USERVER_NAMESPACE_BEGIN
namespace storages::odbc {
namespace detail {
    class ConnectionPtr;
}

/// @brief RAII transaction wrapper, auto-<b>ROLLBACK</b>s on destruction if no
/// prior `Commit`/`Rollback` call was made.
///
/// This type can't be constructed in user code and is always retrieved from
/// storages::odbc::Cluster
class Transaction final {
public:
    explicit Transaction(detail::ConnectionPtr&& connection, engine::Deadline deadline);
    ~Transaction();
    Transaction(const Transaction& other) = delete;
    Transaction(Transaction&& other) noexcept;

    ResultSet Execute(const Query& query) const;

    /// @brief Commit the transaction
    void Commit();

    /// @brief Rollback the transaction
    void Rollback();

private:
    ResultSet DoExecute(const Query& query) const;

    void AssertValid() const;

    utils::FastPimpl<detail::ConnectionPtr, 24, 8> connection_;
    engine::Deadline deadline_;
    tracing::Span span_;
    utils::trx_tracker::TransactionLock trx_lock_;
}
}
USERVER_NAMESPACE_END