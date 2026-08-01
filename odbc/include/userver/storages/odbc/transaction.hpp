#pragma once

/// @file userver/storages/odbc/transaction.hpp

#include <chrono>

#include <userver/engine/deadline.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/datetime/steady_coarse_clock.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/trx_tracker.hpp>

#include <userver/storages/odbc/command_control.hpp>
#include <userver/storages/odbc/impl/parameter.hpp>
#include <userver/storages/odbc/query.hpp>
#include <userver/storages/odbc/result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

namespace detail {
class ConnectionPtr;
class Pool;
}  // namespace detail

/// @brief RAII transaction wrapper, auto-<b>ROLLBACK</b>s on destruction if no
/// prior `Commit`/`Rollback` call was made.
///
/// This type can't be constructed in user code and is always retrieved from
/// storages::odbc::Cluster
class Transaction final {
public:
    explicit Transaction(
        detail::ConnectionPtr&& connection,
        detail::Pool& pool,
        std::chrono::milliseconds network_timeout,
        std::chrono::milliseconds statement_timeout
    );
    ~Transaction();
    Transaction(const Transaction& other) = delete;
    Transaction(Transaction&& other) noexcept;

    /// @brief Execute a statement, binding every argument to an ODBC `?` placeholder.
    template <typename... Args>
    ResultSet Execute(const Query& query, const Args&... args) {
        return Execute(std::nullopt, query, args...);
    }

    /// @brief Execute a statement with per-statement timeout overrides.
    template <typename... Args>
    ResultSet Execute(OptionalCommandControl command_control, const Query& query, const Args&... args) {
        return DoExecute(command_control, query, impl::MakeParameterList(args...));
    }

    /// @brief Commit the transaction
    void Commit();

    /// @brief Rollback the transaction
    void Rollback();

private:
    ResultSet DoExecute(
        OptionalCommandControl command_control,
        const Query& query,
        const impl::ParameterList& parameters
    );
    void AssertValid() const;

    // shared_ptr<Pool>(16) + unique_ptr<Connection>(8) = 24 bytes, align 8
    utils::FastPimpl<detail::ConnectionPtr, 24, 8> connection_;
    detail::Pool* pool_;
    std::chrono::milliseconds network_timeout_;
    std::chrono::milliseconds statement_timeout_;
    utils::datetime::SteadyCoarseClock::time_point start_time_;
    std::chrono::microseconds busy_time_{0};
    tracing::Span span_;
    utils::trx_tracker::TransactionLock trx_lock_;
};

}  // namespace storages::odbc

USERVER_NAMESPACE_END
