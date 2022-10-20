#pragma once

#include <userver/utils/clang_format_workarounds.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace rcu {

class ReadIndicatorLock;

/// @brief A synchronization primitive that protects some data from being
/// modified or deleted as long as there exists at least one reader.
/// @see Based on ideas from https://youtu.be/FtaD0maxwec
class ReadIndicator final {
 public:
  /// @brief Create a new unused instance of `ReadIndicator`
  ReadIndicator() noexcept;

  ReadIndicator(ReadIndicator&&) = delete;
  ReadIndicator& operator=(ReadIndicator&&) = delete;
  ~ReadIndicator();

  /// @brief Mark the indicator as "used" as long as the returned lock is alive
  /// @note The lock must not outlive the `ReadIndicator`
  /// @note The data may still be retired in parallel with a `Lock()` call. Its
  /// actuality must be checked by the ReadIndicator's user after the call.
  ReadIndicatorLock Lock() noexcept;

  /// @returns `true` if there are no locks being held on the `ReadIndicator`
  /// @note This method must only be called after direct access to this
  /// ReadIndicator is closed for readers. Locks acquired during this method
  /// call may or may not be accounted for.
  /// @note This method must be called under the writer lock
  bool IsFree() noexcept;

 private:
  friend class ReadIndicatorLock;

  struct Impl;
  utils::FastPimpl<Impl, 152, 8> impl_;
};

/// @brief Keeps the data protected by a `ReadIndicator` from being retired
class USERVER_NODISCARD ReadIndicatorLock final {
 public:
  /// @brief Produces a `null` instance
  ReadIndicatorLock() noexcept = default;

  ReadIndicatorLock(ReadIndicatorLock&&) noexcept;
  ReadIndicatorLock(const ReadIndicatorLock&) noexcept;
  ReadIndicatorLock& operator=(ReadIndicatorLock&&) noexcept;
  ReadIndicatorLock& operator=(const ReadIndicatorLock&) noexcept;
  ~ReadIndicatorLock();

 private:
  explicit ReadIndicatorLock(ReadIndicator& indicator) noexcept;

  friend class ReadIndicator;

  ReadIndicator* indicator_{nullptr};
};

}  // namespace rcu

USERVER_NAMESPACE_END
