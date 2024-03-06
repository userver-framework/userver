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

  /// @brief The read is done with a relaxed memory order.
  /// @note Due to the underlying implementation being an array of counters,
  /// this function may return logically impossible values if decrements are in
  /// play.
  /// For example, doing Add(1) and Subtract(1) may lead to this
  /// function returning -1 (wrapped).
  std::uint64_t Read() const noexcept;

 private:
  struct Impl;
  utils::FastPimpl<Impl, 32, 8> impl_;
};

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
