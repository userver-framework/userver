#pragma once

#if defined(__cpp_impl_three_way_comparison)
#include <compare>
#endif

/// @file userver/utils/statistics/rate.hpp
/// @brief @copybrief utils::statistics::Rate

#include <cstdint>

#include <userver/formats/serialize/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

// Several monitoring systems can differentiate between plain counter
// and 'rate' counter, where 'rate' values are given special treatment
// with regards to maintaining proper non-negative derivative.
struct Rate {
  using ValueType = std::uint64_t;

  ValueType value;

  inline Rate& operator+=(Rate other) noexcept {
    value += other.value;
    return *this;
  }

#if defined(__cpp_impl_three_way_comparison) && \
    __cpp_impl_three_way_comparison >= 201907L
  auto operator<=>(const Rate& other) const noexcept = default;
#else
  bool operator==(const Rate& other) const noexcept {
    return value == other.value;
  }

  bool operator!=(const Rate& other) const noexcept {
    return !(*this == other);
  }
#endif
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
