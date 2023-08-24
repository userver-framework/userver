#pragma once

/// @file userver/utils/statistics/rate.hpp
/// @brief @copybrief utils::statistics::Rate

#include <cstdint>

#include <userver/formats/serialize/to.hpp>

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

  bool operator==(const Rate& rhs) const noexcept { return value == rhs.value; }

  bool operator!=(const Rate& rhs) const noexcept { return !(*this == rhs); }

  bool operator<(const Rate& rhs) const noexcept { return value < rhs.value; }

  bool operator>(const Rate& rhs) const noexcept { return rhs < *this; }

  bool operator<=(const Rate& rhs) const noexcept { return !(rhs < *this); }

  bool operator>=(const Rate& rhs) const noexcept { return !(*this < rhs); }
};

inline Rate operator+(Rate first, Rate second) noexcept {
  return Rate{first.value + second.value};
}

template <typename ValueType>
ValueType Serialize(const Rate& rate,
                    USERVER_NAMESPACE::formats::serialize::To<ValueType>) {
  using ValueBuilder = typename ValueType::Builder;
  return ValueBuilder{rate.value}.ExtractValue();
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
