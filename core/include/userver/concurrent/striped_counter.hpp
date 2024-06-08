#pragma once

/// @file userver/concurrent/striped_counter.hpp
/// @brief @copybrief concurrent::StripedCounter

#include <cstdint>

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

/// @ingroup userver_concurrency
///
/// @brief A contention-free sharded atomic counter, with memory consumption
/// and read performance traded for write performance. Intended to be used for
/// write-heavy counters, mostly in metrics.
///
/// @note Depending on the underlying platform is implemented either via a
/// single atomic variable, or an 'nproc'-sized array of interference-shielded
/// rseq-based (https://www.phoronix.com/news/Restartable-Sequences-Speed)
/// per-CPU counters.
/// In the second case, read is approx. `nproc` times slower than write.
class StripedCounter final {
 public:
  /// @brief Constructs a zero-initialized counter.
  /// Might allocate up to kDestructiveInterferenceSize (64 bytes for x86_64) *
  /// number of available CPUs bytes.
  StripedCounter();
  ~StripedCounter();

  StripedCounter(const StripedCounter&) = delete;
  StripedCounter& operator=(const StripedCounter&) = delete;

  StripedCounter(StripedCounter&&) = delete;
  StripedCounter& operator=(StripedCounter&&) = delete;

  /// @brief The addition is done with a relaxed memory order.
  void Add(std::uintptr_t value) noexcept;

  /// @brief The subtraction is done with a relaxed memory order.
  void Subtract(std::uintptr_t value) noexcept {
    // Perfectly defined unsigned wrapping
    Add(-value);
  }

  /// @brief Read the the total counter value. The counter uses the full range
  /// of `std::uintptr_t`, using wrap-around when necessary.
  /// @note The read is done with `std::memory_order_acquire`.
  ///
  /// Due to the underlying implementation being an array of counters, this
  /// function may return logically impossible values if `Subtract` is in play.
  /// For example, doing Add(1) and Subtract(1) may lead to this
  /// function returning -1 (wrapped).
  /// With `Subtract`, consider using `NonNegativeRead` instead.
  std::uintptr_t Read() const noexcept;

  /// @brief Read the non-negative total counter value.
  /// @note The read is done with `std::memory_order_acquire`.
  ///
  /// This is almost exactly like `Read`, but for an `Add-Subtract` race,
  /// instead of returning a negative value, this function returns `0`. Consider
  /// this a convenient shortcut to avoid seeing logically impossible negative
  /// values.
  std::uintptr_t NonNegativeRead() const noexcept;

 private:
  struct Impl;
  utils::FastPimpl<Impl, 32, 8> impl_;
};

}  // namespace concurrent

USERVER_NAMESPACE_END
