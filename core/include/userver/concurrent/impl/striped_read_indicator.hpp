#pragma once

#include <cstdint>
#include <limits>

#include <userver/concurrent/striped_counter.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

class StripedReadIndicatorLock;

/// @ingroup userver_concurrency
///
/// @brief Protects some data from being modified or deleted as long as there is
/// at least one reader.
///
/// Under heavy concurrent usage, performs far better than something like
/// a refcount, `Lock-Unlock` is wait-free population oblivious.
///
/// Allocates `16 * N_CORES` bytes, which is more than a simple refcount,
/// of course. Use sparingly and beware of the memory usage.
///
/// Another drawback compared to a conventional refcount is that free-ness is
/// not signaled directly. The only way to await `IsFree` is to check it
/// periodically.
///
/// @see Based on ideas from https://youtu.be/FtaD0maxwec
class StripedReadIndicator final {
 public:
  /// @brief Create a new unused instance of `ReadIndicator`.
  StripedReadIndicator();

  StripedReadIndicator(StripedReadIndicator&&) = delete;
  StripedReadIndicator& operator=(StripedReadIndicator&&) = delete;
  ~StripedReadIndicator();

  /// @brief Mark the indicator as "used" as long as the returned lock is alive.
  /// @note The lock should not outlive the `StripedReadIndicator`.
  /// @note The data may still be retired in parallel with a `Lock()` call.
  /// After calling `Lock`, the reader must check whether the data has been
  /// retired in the meantime.
  /// @note `Lock` uses `std::memory_order_relaxed`, and unlock uses
  /// `std::memory_order_release` to ensure that unlocks don't run ahead
  /// of locks from `IsFree`'s point of view.
  /// @warning Readers must ensure that `Lock` is visible by `IsFree` checks
  /// in other threads when necessary
  /// (which may require `std::atomic_thread_fence(std::memory_order_seq_cst)`.
  /// `Lock` and `IsFree` use weak memory orders, which may lead to `Lock`
  /// unexpectedly not being visible to `IsFree`.
  StripedReadIndicatorLock Lock() noexcept;

  /// @returns `true` if there are no locks held on the `StripedReadIndicator`.
  /// @note `IsFree` should only be called after direct access to this
  /// StripedReadIndicator is closed for readers. Locks acquired during
  /// the `IsFree` call may or may not be accounted for.
  /// @note May sometimes falsely return `false` when the `StripedReadIndicator`
  /// has just become free. Never falsely returns `true`.
  /// @note Uses effectively `std::memory_order_acquire`.
  bool IsFree() const noexcept;

  /// @returns `true` if there are no locks held on any of the `indicators`.
  /// @see IsFree
  template <typename StripedReadIndicatorRange>
  static bool AreAllFree(StripedReadIndicatorRange&& indicators) {
    // See GetActiveCountApprox for implementation strategy explanation.
    std::uintptr_t released = 0;
    for (const auto& indicator : indicators) {
      released += indicator.released_count_.Read();
    }

    std::uintptr_t acquired = 0;
    for (const auto& indicator : indicators) {
      acquired += indicator.acquired_count_.Read();
    }

    UASSERT(acquired - released <=
            std::numeric_limits<std::uintptr_t>::max() / 2);
    return acquired == released;
  }

  /// Get the total amount of `Lock` calls, useful for metrics.
  std::uintptr_t GetAcquireCountApprox() const noexcept;

  /// Get the total amount of `Lock` drops, useful for metrics.
  std::uintptr_t GetReleaseCountApprox() const noexcept;

  /// Get the approximate amount of locks held, useful for metrics.
  std::uintptr_t GetActiveCountApprox() const noexcept;

 private:
  friend class StripedReadIndicatorLock;

  void DoLock() noexcept;
  void DoUnlock() noexcept;

  StripedCounter acquired_count_;
  StripedCounter released_count_;
};

/// @brief Keeps the data protected by a StripedReadIndicator from being retired
class [[nodiscard]] StripedReadIndicatorLock final {
 public:
  /// @brief Produces a `null` instance
  StripedReadIndicatorLock() noexcept = default;

  StripedReadIndicatorLock(StripedReadIndicatorLock&&) noexcept;
  StripedReadIndicatorLock(const StripedReadIndicatorLock&) noexcept;
  StripedReadIndicatorLock& operator=(StripedReadIndicatorLock&&) noexcept;
  StripedReadIndicatorLock& operator=(const StripedReadIndicatorLock&) noexcept;
  ~StripedReadIndicatorLock();

 private:
  explicit StripedReadIndicatorLock(StripedReadIndicator& indicator) noexcept;

  friend class StripedReadIndicator;

  StripedReadIndicator* indicator_{nullptr};
};

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
