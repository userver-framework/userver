#pragma once

#include <cstdint>

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

class StripedCounter final {
 public:
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
  /// @note The read is done with a relaxed memory order.
  ///
  /// Due to the underlying implementation being an array of counters, this
  /// function may return logically impossible values if `Subtract` is in play.
  /// For example, doing Add(1) and Subtract(1) may lead to this
  /// function returning -1 (wrapped).
  /// With `Subtract`, consider using `NonNegativeRead` instead.
  std::uintptr_t Read() const noexcept;

  /// @brief Read the non-negative total counter value.
  /// @note The read is done with a relaxed memory order.
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

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
