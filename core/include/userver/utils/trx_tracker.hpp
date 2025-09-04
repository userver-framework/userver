#pragma once

/// @file
/// @brief Tracking for heavy operations while having active transactions.

#include <optional>
#include <thread>

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
/// and checking for active transactions in heavy operations.
///
/// ## Example usage:
///
/// @snippet utils/trx_tracker_test.cpp  Sample TransactionTracker usage
namespace utils::trx_tracker {

namespace impl {

/// @brief Global enabler for transaction tracker.
class GlobalEnabler final {
public:
    explicit GlobalEnabler(bool enable = true);
    ~GlobalEnabler();

    GlobalEnabler(const GlobalEnabler&) = delete;
    GlobalEnabler& operator=(const GlobalEnabler&) = delete;
};

/// @brief Check if transaction tracker is enabled.
bool IsEnabled() noexcept;

/// @brief Unique ID for every task.
///
/// Sometimes transactions start and end in different coroutines.
/// To prevent transaction from incrementing and decrementing different
/// transaction counters, TransactionLock stores TaskId on Lock and
/// checks that stored TaskId is the same as current TaskId in Unlock.
class TaskId final {
public:
    TaskId();

    bool operator==(const TaskId& other) const;
    bool operator!=(const TaskId& other) const;

private:
    std::thread::id created_thread_id_;
    std::uint64_t thread_local_counter_;
};

}  // namespace impl

/// @brief Class for incrementing and decrementing transaction counter.
class TransactionLock final {
public:
    TransactionLock() = default;
    TransactionLock(const TransactionLock&) = delete;
    TransactionLock(TransactionLock&&) noexcept;
    TransactionLock operator=(const TransactionLock&) = delete;
    TransactionLock& operator=(TransactionLock&&) noexcept;

    /// @brief Decrement transaction counter on destruction.
    ~TransactionLock();

    /// @brief Manually increment transaction counter.
    void Lock() noexcept;

    /// @brief Manually decrement transaction counter.
    void Unlock() noexcept;

private:
    std::optional<impl::TaskId> task_id_;
};

/// @brief Check for active transactions.
void CheckNoTransactions(utils::impl::SourceLocation location = utils::impl::SourceLocation::Current());

/// @overload
void CheckNoTransactions(std::string_view location);

/// @brief Disable check for active transactions.
///
/// To consciously call a heavy operation in active transaction,
/// check can be disabled by creating an instance of this class.
/// Checks will be disabled until every instance either has
/// Reenable() method called or is destroyed.
class CheckDisabler final {
public:
    /// @brief Disable check for active transactions.
    explicit CheckDisabler();

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
