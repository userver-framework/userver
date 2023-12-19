#pragma once

/// @file userver/utils/statistics/rate.hpp
/// @brief @copybrief utils::statistics::Rate

#include <cstdint>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// `Rate` metrics (or "counter" metrics) are metrics that only monotonically
/// increase or are reset to zero on restart. Some monitoring systems give them
/// special treatment with regard to maintaining proper non-negative derivative.
struct Rate {
  using ValueType = std::uint64_t;

  ValueType value{0};

  inline Rate& operator+=(Rate other) noexcept {
    value += other.value;
    return *this;
  }

  explicit operator bool() const noexcept { return value != 0; }

  bool operator==(Rate rhs) const noexcept { return value == rhs.value; }

  bool operator!=(Rate rhs) const noexcept { return !(*this == rhs); }

  bool operator<(Rate rhs) const noexcept { return value < rhs.value; }

  bool operator>(Rate rhs) const noexcept { return rhs < *this; }

  bool operator<=(Rate rhs) const noexcept { return !(rhs < *this); }

  bool operator>=(Rate rhs) const noexcept { return !(*this < rhs); }

  bool operator==(std::uint64_t rhs) const noexcept { return value == rhs; }

  bool operator!=(std::uint64_t rhs) const noexcept { return value == rhs; }

  bool operator<(std::uint64_t rhs) const noexcept { return value < rhs; }

  bool operator>(std::uint64_t rhs) const noexcept { return value > rhs; }

  bool operator<=(std::uint64_t rhs) const noexcept { return value <= rhs; }

  bool operator>=(std::uint64_t rhs) const noexcept { return value >= rhs; }
};

inline Rate operator+(Rate first, Rate second) noexcept {
  return Rate{first.value + second.value};
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
