#pragma once

/// @file userver/storages/odbc/transaction.hpp

#include <chrono>

#include <userver/engine/deadline.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/trx_tracker.hpp>

#include <userver/storages/odbc/result_set.hpp>
#include <userver/storages/odbc/query.hpp>

USERVER_NAMESPACE_BEGIN
namespace storages::odbc {
namespace detail {
    class ConnectionPtr;
    class Pool;
}

/// @brief RAII transaction wrapper, auto-<b>ROLLBACK</b>s on destruction if no
/// prior `Commit`/`Rollback` call was made.
///
/// This type can't be constructed in user code and is always retrieved from
/// storages::odbc::Cluster
class Transaction final {
public:
    explicit Transaction(detail::ConnectionPtr&& connection, detail::Pool& pool, engine::Deadline deadline);
    ~Transaction();
    Transaction(const Transaction& other) = delete;
    Transaction(Transaction&& other) noexcept;

    ResultSet Execute(const Query& query);

    /// @brief Commit the transaction
    void Commit();

    /// @brief Rollback the transaction
    void Rollback();

private:
    void AssertValid() const;

    detail::ConnectionPtr connection_;
    detail::Pool* pool_;
    engine::Deadline deadline_;
    std::chrono::steady_clock::time_point start_time_;
    std::chrono::microseconds busy_time_{0};
    tracing::Span span_;
    utils::trx_tracker::TransactionLock trx_lock_;
};
}  // namespace storages::odbc
USERVER_NAMESPACE_END
