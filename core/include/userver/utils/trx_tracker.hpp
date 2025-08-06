#pragma once

/// @file
/// @brief Tracking for heavy operations while having active transactions.

#include <userver/utils/impl/source_location.hpp>
#include <userver/utils/statistics/rate.hpp>

USERVER_NAMESPACE_BEGIN

/// @brief Tracking for heavy operations while having active transactions.
///
/// Some operations, like http requests, are heavy and can take
/// too long during an incident. If they are called during an active
/// database transaction, connection will be held for longer and
/// connection pool will be exhausted. Transaction tracker prevents this
/// by holding counter of active transactions in TaskLocalVariable
/// and checking for active transactions in heavy operaions.
///
/// ## Example usage:
///
/// @snippet utils/trx_tracker_test.cpp  Sample TransactionTracker usage
namespace utils::trx_tracker {

/// @brief Increment transaction counter.
void StartTransaction();

/// @brief Decrement transaction counter.
///
/// If called before StartTransaction, the behavior is undefined.
void EndTransaction() noexcept;

/// @brief Check for active transactions.
void CheckNoTransactions(utils::impl::SourceLocation location = utils::impl::SourceLocation::Current());

/// @brief Disable check for active transactions.
///
/// To conciously call a heavy operation in active transaction,
/// check can be disabled by creating an instance of this class.
/// Checks will be disabled until every instance either has
/// Reenable() method called or is destroyed.
class CheckDisabler final {
public:
    /// @brief Disable check for active transactions.
    CheckDisabler();

    /// @brief Reenable check for active transactions on destruction.
    ~CheckDisabler();

    CheckDisabler(const CheckDisabler&) = delete;
    CheckDisabler(CheckDisabler&&) = delete;
    CheckDisabler operator=(const CheckDisabler&) = delete;
    CheckDisabler operator=(CheckDisabler&&) = delete;

    /// @brief Manually reenable check for active transactions.
    void Reenable() noexcept;

private:
    bool reenabled_ = false;
};

/// @brief Statistics for transaction tracker.
struct TransactionTrackerStatistics final {
    /// @brief How many times check for active transactions was triggered.
    utils::statistics::Rate triggers{0};
};

/// @brief Get statistics for transaction tracker.
TransactionTrackerStatistics GetStatistics() noexcept;

/// @brief Reset statistics for transaction tracker.
void ResetStatistics();

}  // namespace utils::trx_tracker

USERVER_NAMESPACE_END
