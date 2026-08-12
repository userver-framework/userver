#pragma once

/// @file userver/engine/shared_mutex.hpp
/// @brief @copybrief engine::SharedMutex

#include <userver/engine/condition_variable.hpp>
#include <userver/engine/impl/actor.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/semaphore.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @ingroup userver_concurrency
///
/// @brief std::shared_mutex replacement for asynchronous tasks.
///
/// Ignores task cancellations (succeeds even if the current task is cancelled).
///
/// Writers (unique locks) have priority over readers (shared locks),
/// thus new shared lock waits for the pending writes to finish, which in turn
/// waits for existing existing shared locks to unlock first.
///
/// ## Example usage:
///
/// @snippet engine/shared_mutex_test.cpp  Sample engine::SharedMutex usage
///
/// @see @ref scripts/docs/en/userver/synchronization.md
class SharedMutex final {
public:
    SharedMutex();
    ~SharedMutex();

    SharedMutex(const SharedMutex&) = delete;
    SharedMutex(SharedMutex&&) = delete;
    SharedMutex& operator=(const SharedMutex&) = delete;
    SharedMutex& operator=(SharedMutex&&) = delete;

    /// Locks the mutex for unique ownership. Blocks current task if the
    /// mutex is locked by another task for reading or writing.
    ///
    /// @note The method waits for the mutex even if the current task is cancelled.
    void lock();

    /// Unlocks the mutex for unique ownership. Before calling this method the
    /// the mutex should be locked for unique ownership by current task.
    ///
    /// @note the order of tasks to unblock is unspecified. Any code assuming
    /// any specific order (e.g. FIFO) is incorrect and should be fixed.
    void unlock();

    /// Tries to lock the mutex for unique ownership without blocking the task, returns true if succeeded.
    [[nodiscard]] bool try_lock();

    /// Tries to lock the mutex for unique ownership in specified duration.
    /// Blocks current task if the mutex is locked by another task up to the provided duration.
    ///
    /// @returns true if the locking succeeded
    template <typename Rep, typename Period>
    [[nodiscard]] bool try_lock_for(const std::chrono::duration<Rep, Period>&);

    /// Tries to lock the mutex for unique ownership till specified time point.
    /// Blocks current task if the mutex is locked by another task up to the provided duration.
    ///
    /// @returns true if the locking succeeded
    template <typename Clock, typename Duration>
    [[nodiscard]] bool try_lock_until(const std::chrono::time_point<Clock, Duration>&);

    /// @overload
    [[nodiscard]] bool try_lock_until(Deadline deadline);

    /// Locks the mutex for shared access. Blocks the current task if the mutex
    /// is already locked by another task for writing or if there are writers waiting.
    ///
    /// @note The method waits for the mutex even if the current task is cancelled.
    void lock_shared();

    /// Unlocks the mutex for shared ownership. Before calling this method the
    /// mutex should be locked for shared ownership by current task.
    ///
    /// @note the order of tasks to unblock is unspecified. Any code assuming
    /// any specific order (e.g. FIFO) is incorrect and should be fixed.
    void unlock_shared();

    /// Atomically converts the ownership from exclusive to shared.
    ///
    /// @note Does not block.
    void unlock_and_lock_shared();

    /// Tries to lock the mutex for shared ownership without blocking the
    /// task, returns true if succeeded.
    [[nodiscard]] bool try_lock_shared();

    /// Tries to lock the mutex for shared ownership in specified duration.
    /// Blocks current task if the mutex is locked by another task up to the provided duration.
    ///
    /// @returns true if the locking succeeded
    template <typename Rep, typename Period>
    [[nodiscard]] bool try_lock_shared_for(const std::chrono::duration<Rep, Period>&);

    /// Tries to lock the mutex for shared ownership till specified time point.
    /// Blocks current task if the mutex is locked by another task up to the provided duration.
    ///
    /// @returns true if the locking succeeded
    template <typename Clock, typename Duration>
    [[nodiscard]] bool try_lock_shared_until(const std::chrono::time_point<Clock, Duration>&);

    /// @overload
    [[nodiscard]] bool try_lock_shared_until(Deadline deadline);

private:
    bool HasRegisteredWriter() const noexcept;

    bool WaitForNoRegisteredWriters(Deadline deadline);

    enum class DeadlockDetectorTrackingStatus : std::uint8_t { kUntracked, kTracked };

    void UnregisterWriter(DeadlockDetectorTrackingStatus tracking_status);

    /* Semaphore can be get by 1 or by SIZE_MAX.
     * 1 = reader, SIZE_MAX = writer.
     *
     * Three possible cases:
     * 1) semaphore is free
     * 2) there are readers in critical section (any count)
     * 3) there is a single writer in critical section
     */
    Semaphore semaphore_;

    /* Readers don't try to hold semaphore_ if there is at least one
     * registered writer => writers don't starve.
     */
    std::atomic<ssize_t> registered_writers_count_;
    Mutex registered_writers_count_mutex_;
    ConditionVariable registered_writers_count_cv_;

    struct RegisteredWritersActor final : public impl::deadlock_detector::Actor {
        utils::StringLiteral GetActorType() const override;
    };

    struct MutexActor final : public impl::deadlock_detector::Actor {
        utils::StringLiteral GetActorType() const override;
    };

    // An identity actor for registered (waiting and active) writers.
    // Used to track dependencies between shared readers and registered writers in deadlock detector.
    // While there are registered writers owning this actor all shared readers will wait for it.
    RegisteredWritersActor registered_writers_actor_;
    // An identity actor for the mutex itself.
    // Used to track ownership of the mutex and waits on the mutex in deadlock detector.
    // While there is an active writer or a set of active shared readers owning this actor
    // other writers will waiting for it.
    MutexActor mutex_actor_;
};

template <typename Rep, typename Period>
bool SharedMutex::try_lock_for(const std::chrono::duration<Rep, Period>& duration) {
    return try_lock_until(Deadline::FromDuration(duration));
}

template <typename Rep, typename Period>
bool SharedMutex::try_lock_shared_for(const std::chrono::duration<Rep, Period>& duration) {
    return try_lock_shared_until(Deadline::FromDuration(duration));
}

template <typename Clock, typename Duration>
bool SharedMutex::try_lock_until(const std::chrono::time_point<Clock, Duration>& until) {
    return try_lock_until(Deadline::FromTimePoint(until));
}

template <typename Clock, typename Duration>
bool SharedMutex::try_lock_shared_until(const std::chrono::time_point<Clock, Duration>& until) {
    return try_lock_shared_until(Deadline::FromTimePoint(until));
}

}  // namespace engine

USERVER_NAMESPACE_END
