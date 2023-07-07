#pragma once

/// @file userver/decimal64/decimal64.hpp
/// @brief Decimal data type for fixed-point arithmetic

// Original source taken from https://github.com/vpiotr/decimal_for_cpp
// Original license:
//
// ==================================================================
// Name:        decimal.h
// Purpose:     Decimal data type support, for COBOL-like fixed-point
//              operations on currency values.
// Author:      Piotr Likus
// Created:     03/01/2011
// Modified:    23/09/2018
// Version:     1.16
// Licence:     BSD
// ==================================================================

#include <array>
#include <cassert>
#include <cstdint>
#include <ios>
#include <iosfwd>
#include <istream>
#include <limits>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include <fmt/compile.h>
#include <fmt/format.h>

#include <userver/decimal64/format_options.hpp>
#include <userver/formats/common/meta.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/flags.hpp>
#include <userver/utils/meta_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

class LogHelper;

}  // namespace logging

/// Fixed-point decimal data type and related functions
namespace decimal64 {

/// The base class for Decimal-related exceptions
class DecimalError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

/// Thrown on all errors related to parsing `Decimal` from string
class ParseError : public DecimalError {
 public:
  using DecimalError::DecimalError;
};

/// Thrown on overflow in `Decimal` arithmetic
class OutOfBoundsError : public DecimalError {
 public:
  OutOfBoundsError();
};

/// Thrown on division by zero in `Decimal` arithmetic
class DivisionByZeroError : public DecimalError {
 public:
  DivisionByZeroError();
};

namespace impl {

inline constexpr auto kMaxInt64 = std::numeric_limits<int64_t>::max();
inline constexpr auto kMinInt64 = std::numeric_limits<int64_t>::min();

template <typename T>
using EnableIfInt = std::enable_if_t<meta::kIsInteger<T>, int>;

template <typename T>
using EnableIfFloat = std::enable_if_t<std::is_floating_point_v<T>, int>;

template <int MaxExp>
constexpr std::array<int64_t, MaxExp + 1> PowSeries(int64_t base) {
  int64_t pow = 1;
  std::array<int64_t, MaxExp + 1> result{};
  for (int i = 0; i < MaxExp; ++i) {
    result[i] = pow;
    if (pow > kMaxInt64 / base) {
      throw OutOfBoundsError();
    }
    pow *= base;
  }
  result[MaxExp] = pow;
  return result;
}

inline constexpr int kMaxDecimalDigits = 18;
inline constexpr auto kPowSeries10 = PowSeries<kMaxDecimalDigits>(10);

// Check that kMaxDecimalDigits is indeed max integer x such that 10^x is valid
static_assert(kMaxInt64 / 10 < kPowSeries10[kMaxDecimalDigits]);

template <typename RoundPolicy>
constexpr int64_t Div(int64_t nominator, int64_t denominator,
                      bool extra_odd_quotient = false) {
  // RoundPolicies don't protect against arithmetic errors
  if (denominator == 0) throw DivisionByZeroError();
  if (denominator == -1) {
    if (nominator == kMinInt64) throw OutOfBoundsError();
    return -nominator;  // RoundPolicies behave badly for denominator == -1
  }

  return RoundPolicy::DivRounded(nominator, denominator, extra_odd_quotient);
}

// result = (value1 * value2) / divisor
template <typename RoundPolicy>
constexpr int64_t MulDiv(int64_t value1, int64_t value2, int64_t divisor) {
  if (divisor == 0) throw DivisionByZeroError();

#if __x86_64__ || __ppc64__ || __aarch64__
  using LongInt = __int128_t;
  static_assert(sizeof(void*) == 8);
#else
  using LongInt = int64_t;
  static_assert(sizeof(void*) == 4);
#endif

  LongInt prod{};
  if constexpr (sizeof(void*) == 4) {
    if (__builtin_mul_overflow(static_cast<LongInt>(value1),
                               static_cast<LongInt>(value2), &prod)) {
      throw OutOfBoundsError();
    }
  } else {
    prod = static_cast<LongInt>(value1) * value2;
  }
  const auto whole = prod / divisor;
  const auto rem = static_cast<int64_t>(prod % divisor);

  if (whole <= kMinInt64 || whole >= kMaxInt64) throw OutOfBoundsError();

  const auto whole64 = static_cast<int64_t>(whole);
  const bool extra_odd_quotient = whole64 % 2 != 0;
  const int64_t rem_divided =
      Div<RoundPolicy>(rem, divisor, extra_odd_quotient);
  UASSERT(rem_divided == -1 || rem_divided == 0 || rem_divided == 1);

  return whole64 + rem_divided;
}

constexpr int Sign(int64_t value) { return (value > 0) - (value < 0); }

// Needed because std::abs is not constexpr
constexpr int64_t Abs(int64_t value) {
  if (value == kMinInt64) throw OutOfBoundsError();
  return value >= 0 ? value : -value;
}

// Needed because std::abs is not constexpr
// Insignificantly less performant
template <typename T>
constexpr int64_t Abs(T value) {
  static_assert(std::is_floating_point_v<T>);
  return value >= 0 ? value : -value;
}

// Needed because std::floor is not constexpr
// Insignificantly less performant
template <typename T>
constexpr int64_t Floor(T value) {
  if (static_cast<int64_t>(value) <= value) {  // whole or positive
    return static_cast<int64_t>(value);
  } else {
    return static_cast<int64_t>(value) - 1;
  }
}

// Needed because std::ceil is not constexpr
// Insignificantly less performant
template <typename T>
constexpr int64_t Ceil(T value) {
  if (static_cast<int64_t>(value) >= value) {  // whole or negative
    return static_cast<int64_t>(value);
  } else {
    return static_cast<int64_t>(value) + 1;
  }
}

template <typename Int>
constexpr int64_t ToInt64(Int value) {
  static_assert(meta::kIsInteger<Int>);
  static_assert(sizeof(Int) <= sizeof(int64_t));

  if constexpr (sizeof(Int) == sizeof(int64_t)) {
    if (value > kMaxInt64) throw OutOfBoundsError();
  }
  return static_cast<int64_t>(value);
}

class HalfUpPolicy final {
 public:
  // returns 'true' iff 'abs' should be rounded away from 0
  template <typename T>
  [[nodiscard]] static constexpr bool ShouldRoundAwayFromZero(T abs) {
    const T abs_remainder = abs - static_cast<int64_t>(abs);
    return abs_remainder >= 0.5;
  }

  // returns 'true' iff 'a / b' should be rounded away from 0
  static constexpr bool ShouldRoundAwayFromZeroDiv(
      int64_t a, int64_t b, bool /*extra_odd_quotient*/) {
    const int64_t abs_a = impl::Abs(a);
    const int64_t abs_b = impl::Abs(b);
    const int64_t half_b = abs_b / 2;
    const int64_t abs_remainder = abs_a % abs_b;
    return abs_b % 2 == 0 ? abs_remainder >= half_b : abs_remainder > half_b;
  }
};

class HalfDownPolicy final {
 public:
  // returns 'true' iff 'abs' should be rounded away from 0
  template <typename T>
  [[nodiscard]] static constexpr bool ShouldRoundAwayFromZero(T abs) {
    const T abs_remainder = abs - static_cast<int64_t>(abs);
    return abs_remainder > 0.5;
  }

  // returns 'true' iff 'a / b' should be rounded away from 0
  static constexpr bool ShouldRoundAwayFromZeroDiv(
      int64_t a, int64_t b, bool /*extra_odd_quotient*/) {
    const int64_t abs_a = impl::Abs(a);
    const int64_t abs_b = impl::Abs(b);
    const int64_t half_b = abs_b / 2;
    const int64_t abs_remainder = abs_a % abs_b;
    return abs_remainder > half_b;
  }
};

class HalfEvenPolicy final {
 public:
  // returns 'true' iff 'abs' should be rounded away from 0
  template <typename T>
  [[nodiscard]] static constexpr bool ShouldRoundAwayFromZero(T abs) {
    const T abs_remainder = abs - static_cast<int64_t>(abs);
    return abs_remainder == 0.5 ? impl::Floor(abs) % 2 != 0
                                : abs_remainder > 0.5;
  }

  // returns 'true' iff 'a / b' should be rounded away from 0
  static constexpr bool ShouldRoundAwayFromZeroDiv(int64_t a, int64_t b,
                                                   bool extra_odd_quotient) {
    const int64_t abs_a = impl::Abs(a);
    const int64_t abs_b = impl::Abs(b);
    const int64_t half_b = abs_b / 2;
    const int64_t abs_remainder = abs_a % abs_b;
    return (abs_b % 2 == 0 && abs_remainder == half_b)
               ? ((abs_a / abs_b) % 2 == 0) == extra_odd_quotient
               : abs_remainder > half_b;
  }
};

template <typename HalfPolicy>
class HalfRoundPolicyBase {
 public:
  template <typename T>
  [[nodiscard]] static constexpr int64_t Round(T value) {
    if ((value >= 0.0) == HalfPolicy::ShouldRoundAwayFromZero(value)) {
      return impl::Ceil(value);
    } else {
      return impl::Floor(value);
    }
  }

  [[nodiscard]] static constexpr int64_t DivRounded(int64_t a, int64_t b,
                                                    bool extra_odd_quotient) {
    if (HalfPolicy::ShouldRoundAwayFromZeroDiv(a, b, extra_odd_quotient)) {
      const auto quotient_sign = impl::Sign(a) * impl::Sign(b);
      return (a / b) + quotient_sign;  // round away from 0
    } else {
      return a / b;  // round towards 0
    }
  }
};

}  // namespace impl

/// A fast, constexpr-friendly power of 10
constexpr int64_t Pow10(int exp) {
  if (exp < 0 || exp > impl::kMaxDecimalDigits) {
    throw std::runtime_error("Pow10: invalid power of 10");
  }
  return impl::kPowSeries10[static_cast<size_t>(exp)];
}

/// A guaranteed-compile-time power of 10
template <int Exp>
inline constexpr int64_t kPow10 = Pow10(Exp);

/// @brief Default rounding. Fast, rounds to nearest.
///
/// On 0.5, rounds away from zero. Also, sometimes rounds up numbers
/// in the neighborhood of 0.5, e.g. 0.49999999999999994 -> 1.
class DefRoundPolicy final {
 public:
  template <typename T>
  [[nodiscard]] static constexpr int64_t Round(T value) {
    return static_cast<int64_t>(value + (value < 0 ? -0.5 : 0.5));
  }

  [[nodiscard]] static constexpr int64_t DivRounded(
      int64_t a, int64_t b, bool /*extra_odd_quotient*/) {
    const int64_t divisor_corr = impl::Abs(b / 2);
    if (a >= 0) {
      if (impl::kMaxInt64 - a < divisor_corr) throw OutOfBoundsError();
      return (a + divisor_corr) / b;
    } else {
      if (-(impl::kMinInt64 - a) < divisor_corr) throw OutOfBoundsError();
      return (a - divisor_corr) / b;
    }
  }
};

/// Round to nearest, 0.5 towards zero
class HalfDownRoundPolicy final
    : public impl::HalfRoundPolicyBase<impl::HalfDownPolicy> {};

/// Round to nearest, 0.5 away from zero
class HalfUpRoundPolicy final
    : public impl::HalfRoundPolicyBase<impl::HalfUpPolicy> {};

/// Round to nearest, 0.5 towards number with even last digit
class HalfEvenRoundPolicy final
    : public impl::HalfRoundPolicyBase<impl::HalfEvenPolicy> {};

/// Round towards +infinity
class CeilingRoundPolicy {
 public:
  template <typename T>
  [[nodiscard]] static constexpr int64_t Round(T value) {
    return impl::Ceil(value);
  }

  [[nodiscard]] static constexpr int64_t DivRounded(
      int64_t a, int64_t b, bool /*extra_odd_quotient*/) {
    const bool quotient_positive = (a >= 0) == (b >= 0);
    return (a / b) + (a % b != 0 && quotient_positive);
  }
};

/// Round towards -infinity
class FloorRoundPolicy final {
 public:
  template <typename T>
  [[nodiscard]] static constexpr int64_t Round(T value) {
    return impl::Floor(value);
  }

  [[nodiscard]] static constexpr int64_t DivRounded(
      int64_t a, int64_t b, bool /*extra_odd_quotient*/) {
    const bool quotient_negative = (a < 0) != (b < 0);
    return (a / b) - (a % b != 0 && quotient_negative);
  }
};

/// Round towards zero. The fastest rounding.
class RoundDownRoundPolicy final {
 public:
  template <typename T>
  [[nodiscard]] static constexpr int64_t Round(T value) {
    return static_cast<int64_t>(value);
  }

  [[nodiscard]] static constexpr int64_t DivRounded(
      int64_t a, int64_t b, bool /*extra_odd_quotient*/) {
    return a / b;
  }
};

/// Round away from zero
class RoundUpRoundPolicy final {
 public:
  template <typename T>
  [[nodiscard]] static constexpr int64_t Round(T value) {
    if (value >= 0.0) {
      return impl::Ceil(value);
    } else {
      return impl::Floor(value);
    }
  }

  [[nodiscard]] static constexpr int64_t DivRounded(
      int64_t a, int64_t b, bool /*extra_odd_quotient*/) {
    const auto quotient_sign = impl::Sign(a) * impl::Sign(b);
    return (a / b) + (a % b != 0) * quotient_sign;
  }
};

/// @ingroup userver_containers
///
/// @brief Fixed-point decimal data type for use in deterministic calculations,
/// oftentimes involving money
///
/// @tparam Prec The number of fractional digits
/// @tparam RoundPolicy Specifies how to round in lossy operations
///
/// Decimal is internally represented as `int64_t`. It means that it can be
/// passed around by value. It also means that operations with huge
/// numbers can overflow and trap. For example, with `Prec == 6`, the maximum
/// representable number is about 10 trillion.
///
/// Decimal should be serialized and stored as a string, NOT as `double`. Use
/// `Decimal{str}` constructor (or `Decimal::FromStringPermissive` if rounding
/// is allowed) to read a `Decimal`, and `ToString(dec)`
/// (or `ToStringTrailingZeros(dec)`/`ToStringFixed<N>(dec)`) to write a
/// `Decimal`.
///
/// Use arithmetic with caution! Multiplication and division operations involve
/// rounding. You may want to cast to `Decimal` with another `Prec`
/// or `RoundPolicy` beforehand. For that purpose you can use
/// `decimal64::decimal_cast<NewDec>(dec)`.
///
/// Usage example:
/// @code{.cpp}
/// // create a single alias instead of specifying Decimal everywhere
/// using Money = decimal64::Decimal<4, decimal64::HalfEvenRoundPolicy>;
///
/// std::vector<std::string> cart = ...;
/// Money sum{0};
/// for (const std::string& cost_string : cart) {
///   // or use FromStringPermissive to enable rounding
///   sum += Money{cost_string};
/// }
/// return ToString(sum);
/// @endcode
template <int Prec, typename RoundPolicy_ = DefRoundPolicy>
class Decimal {
 public:
  /// The number of fractional digits
  static constexpr int kDecimalPoints = Prec;

  /// Specifies how to round in lossy operations
  using RoundPolicy = RoundPolicy_;

  /// The denominator of the decimal fraction
  static constexpr int64_t kDecimalFactor = kPow10<Prec>;

  /// Zero by default
  constexpr Decimal() noexcept = default;

  /// @brief Convert from an integer
  template <typename Int, impl::EnableIfInt<Int> = 0>
  explicit constexpr Decimal(Int value)
      : Decimal(FromDecimal(Decimal<0>::FromUnbiased(impl::ToInt64(value)))) {}

  /// @brief Convert from a string
  ///
  /// The string must match the following regexp exactly:
  ///
  ///     [+-]?\d+(\.\d+)?
  ///
  /// No extra characters, including spaces, are allowed. Extra leading
  /// and trailing zeros (within `Prec`) are discarded. Input containing more
  /// fractional digits that `Prec` is not allowed (no implicit rounding).
  ///
  /// @throw decimal64::ParseError on invalid input
  /// @see FromStringPermissive
  explicit constexpr Decimal(std::string_view value);

  /// @brief Lossy conversion from a floating-point number
  ///
  /// To somewhat resist the accumulated error, the number is always rounded
  /// to the nearest Decimal, regardless of `RoundPolicy`.
  ///
  /// @warning Prefer storing and sending `Decimal` as string, and performing
  /// the computations between `Decimal`s.
  template <typename T>
  static constexpr Decimal FromFloatInexact(T value) {
    static_assert(std::is_floating_point_v<T>);
    const auto unbiased_float =
        static_cast<long double>(value) * kDecimalFactor;
    if (unbiased_float < impl::kMinInt64 + 1 ||
        unbiased_float > impl::kMaxInt64 - 1) {
      throw OutOfBoundsError();
    }
    return FromUnbiased(DefRoundPolicy::Round(unbiased_float));
  }

  /// @brief Convert from a string, allowing rounding, spaces and boundary dot
  ///
  /// In addition to the `Decimal(str)` constructor, allows:
  /// - rounding (as per `RoundPolicy`), e.g. "12.3456789" with `Prec == 2`
  /// - space characters, e.g. " \t42  \n"
  /// - leading and trailing dot, e.g. "5." and ".5"
  ///
  /// @throw decimal64::ParseError on invalid input
  /// @see Decimal(std::string_view)
  static constexpr Decimal FromStringPermissive(std::string_view input);

  /// @brief Reconstruct from the internal representation, as acquired
  /// with `AsUnbiased`
  ///
  /// The Decimal value will be equal to `value/kDecimalFactor`.
  ///
  /// @see AsUnbiased
  static constexpr Decimal FromUnbiased(int64_t value) noexcept {
    Decimal result;
    result.value_ = value;
    return result;
  }

  /// @brief Convert from `original_unbiased / 10^original_precision`, rounding
  /// according to `RoundPolicy` if necessary
  ///
  /// Usage examples:
  ///
  ///     Decimal<4>::FromBiased(123, 6) -> 0.0001
  ///     Decimal<4>::FromBiased(123, 2) -> 1.23
  ///     Decimal<4>::FromBiased(123, -1) -> 1230
  ///
  /// @param original_unbiased The original mantissa
  /// @param original_precision The original precision (negated exponent)
  static constexpr Decimal FromBiased(int64_t original_unbiased,
                                      int original_precision) {
    const int exponent_for_pack = Prec - original_precision;

    if (exponent_for_pack >= 0) {
      return FromUnbiased(original_unbiased) * Pow10(exponent_for_pack);
    } else {
      return FromUnbiased(
          impl::Div<RoundPolicy>(original_unbiased, Pow10(-exponent_for_pack)));
    }
  }

  /// @brief Assignment from another `Decimal`
  ///
  /// The assignment is allowed as long as `RoundPolicy` is the same. Rounding
  /// will be performed according to `RoundPolicy` if necessary.
  template <int Prec2>
  Decimal& operator=(Decimal<Prec2, RoundPolicy> rhs) {
    *this = FromDecimal(rhs);
    return *this;
  }

  /// @brief Assignment from an integer
  template <typename Int, impl::EnableIfInt<Int> = 0>
  constexpr Decimal& operator=(Int rhs) {
    *this = Decimal{rhs};
    return *this;
  }

  constexpr bool operator==(Decimal rhs) const { return value_ == rhs.value_; }

  constexpr bool operator!=(Decimal rhs) const { return value_ != rhs.value_; }

  constexpr bool operator<(Decimal rhs) const { return value_ < rhs.value_; }

  constexpr bool operator<=(Decimal rhs) const { return value_ <= rhs.value_; }

  constexpr bool operator>(Decimal rhs) const { return value_ > rhs.value_; }

  constexpr bool operator>=(Decimal rhs) const { return value_ >= rhs.value_; }

  constexpr Decimal operator+() const { return *this; }

  constexpr Decimal operator-() const {
    if (value_ == impl::kMinInt64) throw OutOfBoundsError();
    return FromUnbiased(-value_);
  }

  template <int Prec2>
  constexpr auto operator+(Decimal<Prec2, RoundPolicy> rhs) const {
    if constexpr (Prec2 > Prec) {
      return Decimal<Prec2, RoundPolicy>::FromDecimal(*this) + rhs;
    } else if constexpr (Prec2 < Prec) {
      return *this + FromDecimal(rhs);
    } else {
      int64_t result{};
      if (__builtin_add_overflow(AsUnbiased(), rhs.AsUnbiased(), &result)) {
        throw OutOfBoundsError();
      }
      return FromUnbiased(result);
    }
  }

  template <typename Int, impl::EnableIfInt<Int> = 0>
  constexpr Decimal operator+(Int rhs) const {
    return *this + Decimal{rhs};
  }

  template <typename Int, impl::EnableIfInt<Int> = 0>
  friend constexpr Decimal operator+(Int lhs, Decimal rhs) {
    return Decimal{lhs} + rhs;
  }

  template <int Prec2>
  constexpr Decimal& operator+=(Decimal<Prec2, RoundPolicy> rhs) {
    static_assert(Prec2 <= Prec,
                  "Implicit cast to Decimal of lower precision in assignment");
    *this = *this + rhs;
    return *this;
  }

  template <typename Int, impl::EnableIfInt<Int> = 0>
  constexpr Decimal& operator+=(Int rhs) {
    *this = *this + rhs;
    return *this;
  }

  template <int Prec2>
  constexpr auto operator-(Decimal<Prec2, RoundPolicy> rhs) const {
    if constexpr (Prec2 > Prec) {
      return Decimal<Prec2, RoundPolicy>::FromDecimal(*this) - rhs;
    } else if constexpr (Prec2 < Prec) {
      return *this - FromDecimal(rhs);
    } else {
      int64_t result{};
      if (__builtin_sub_overflow(AsUnbiased(), rhs.AsUnbiased(), &result)) {
        throw OutOfBoundsError();
      }
      return FromUnbiased(result);
    }
  }

  template <typename Int, impl::EnableIfInt<Int> = 0>
  constexpr Decimal operator-(Int rhs) const {
    return *this - Decimal{rhs};
  }

  template <typename Int, impl::EnableIfInt<Int> = 0>
  friend constexpr Decimal operator-(Int lhs, Decimal rhs) {
    return Decimal{lhs} - rhs;
  }

  template <int Prec2>
  constexpr Decimal& operator-=(Decimal<Prec2, RoundPolicy> rhs) {
    static_assert(Prec2 <= Prec,
                  "Implicit cast to Decimal of lower precision in assignment");
    *this = *this - rhs;
    return *this;
  }

  template <typename Int, impl::EnableIfInt<Int> = 0>
  constexpr Decimal& operator-=(Int rhs) {
    *this = *this - rhs;
    return *this;
  }

  template <typename Int, typename = impl::EnableIfInt<Int>>
  constexpr Decimal operator*(Int rhs) const {
    int64_t result{};
    if (rhs > impl::kMaxInt64 ||
        __builtin_mul_overflow(value_, static_cast<int64_t>(rhs), &result)) {
      throw OutOfBoundsError();
    }
    return FromUnbiased(result);
  }

  template <typename Int, impl::EnableIfInt<Int> = 0>
  friend constexpr Decimal operator*(Int lhs, Decimal rhs) {
    return rhs * lhs;
  }

  template <typename Int, impl::EnableIfInt<Int> = 0>
  constexpr Decimal& operator*=(Int rhs) {
    *this = *this * rhs;
    return *this;
  }

  template <int Prec2>
  constexpr Decimal operator*(Decimal<Prec2, RoundPolicy> rhs) const {
    return FromUnbiased(impl::MulDiv<RoundPolicy>(
        AsUnbiased(), rhs.AsUnbiased(), kPow10<Prec2>));
  }

  template <int Prec2>
  constexpr Decimal& operator*=(Decimal<Prec2, RoundPolicy> rhs) {
    *this = *this * rhs;
    return *this;
  }

  template <typename Int, typename = impl::EnableIfInt<Int>>
  constexpr Decimal operator/(Int rhs) const {
    return FromUnbiased(impl::Div<RoundPolicy>(AsUnbiased(), rhs));
  }

  template <typename Int, typename = impl::EnableIfInt<Int>>
  friend constexpr Decimal operator/(Int lhs, Decimal rhs) {
    return Decimal{lhs} / rhs;
  }

  template <typename Int, typename = impl::EnableIfInt<Int>>
  constexpr Decimal& operator/=(Int rhs) {
    *this = *this / rhs;
    return *this;
  }

  template <int Prec2>
  constexpr Decimal operator/(Decimal<Prec2, RoundPolicy> rhs) const {
    return FromUnbiased(impl::MulDiv<RoundPolicy>(AsUnbiased(), kPow10<Prec2>,
                                                  rhs.AsUnbiased()));
  }

  template <int Prec2>
  constexpr Decimal& operator/=(Decimal<Prec2, RoundPolicy> rhs) {
    *this = *this / rhs;
    return *this;
  }

  /// Returns one of {-1, 0, +1}, depending on the sign of the `Decimal`
  constexpr int Sign() const { return impl::Sign(value_); }

  /// Returns the absolute value of the `Decimal`
  constexpr Decimal Abs() const { return FromUnbiased(impl::Abs(value_)); }

  /// Rounds `this` to the nearest multiple of `base` according to `RoundPolicy`
  constexpr Decimal RoundToMultipleOf(Decimal base) const {
    if (base < Decimal{0}) throw OutOfBoundsError();
    return *this / base.AsUnbiased() * base.AsUnbiased();
  }

  /// Returns the value rounded to integer using the active rounding policy
  constexpr int64_t ToInteger() const {
    return impl::Div<RoundPolicy>(value_, kDecimalFactor);
  }

  /// @brief Returns the value converted to `double`
  ///
  /// @warning Operations with `double`, and even the returned value,
  /// is inexact. Prefer storing and sending `Decimal` as string, and performing
  /// the computations between `Decimal`s.
  ///
  /// @see FromFloatInexact
  constexpr double ToDoubleInexact() const {
    return static_cast<double>(value_) / kDecimalFactor;
  }

  /// @brief Retrieve the internal representation
  ///
  /// The internal representation of `Decimal` is `real_value * kDecimalFactor`.
  /// Use for storing the value of Decimal efficiently when `Prec` is guaranteed
  /// not to change.
  ///
  /// @see FromUnbiased
  constexpr int64_t AsUnbiased() const { return value_; }

 private:
  template <int Prec2, typename RoundPolicy2>
  static constexpr Decimal FromDecimal(Decimal<Prec2, RoundPolicy2> source) {
    if constexpr (Prec > Prec2) {
      int64_t result{};
      if (__builtin_mul_overflow(source.AsUnbiased(), kPow10<Prec - Prec2>,
                                 &result)) {
        throw OutOfBoundsError();
      }
      return FromUnbiased(result);
    } else if constexpr (Prec < Prec2) {
      return FromUnbiased(
          impl::Div<RoundPolicy>(source.AsUnbiased(), kPow10<Prec2 - Prec>));
    } else {
      return FromUnbiased(source.AsUnbiased());
    }
  }

  template <int Prec2, typename RoundPolicy2>
  friend class Decimal;

  template <typename T, int OldPrec, typename OldRound>
  friend constexpr T decimal_cast(Decimal<OldPrec, OldRound> arg);

  int64_t value_{0};
};

namespace impl {

template <typename T>
struct IsDecimal : std::false_type {};

template <int Prec, typename RoundPolicy>
struct IsDecimal<Decimal<Prec, RoundPolicy>> : std::true_type {};

}  // namespace impl

/// `true` if the type is an instantiation of `Decimal`
template <typename T>
inline constexpr bool kIsDecimal = impl::IsDecimal<T>::value;

/// @brief Cast one `Decimal` to another `Decimal` type
///
/// When casting to a `Decimal` with a lower `Prec`, rounding is performed
/// according to the new `RoundPolicy`.
///
/// Usage example:
/// @code{.cpp}
/// using Money = decimal64::Decimal<4>;
/// using Discount = decimal64::Decimal<4, FloorRoundPolicy>;
///
/// Money cost = ...;
/// auto discount = decimal64::decimal_cast<Discount>(cost) * Discount{"0.05"};
/// @endcode
template <typename T, int OldPrec, typename OldRound>
constexpr T decimal_cast(Decimal<OldPrec, OldRound> arg) {
  static_assert(kIsDecimal<T>);
  return T::FromDecimal(arg);
}

namespace impl {

// FromUnpacked<Decimal<4>>(12, 34) -> 12.0034
// FromUnpacked<Decimal<4>>(-12, -34) -> -12.0034
// FromUnpacked<Decimal<4>>(0, -34) -> -0.0034
template <int Prec, typename RoundPolicy>
constexpr Decimal<Prec, RoundPolicy> FromUnpacked(int64_t before,
                                                  int64_t after) {
  using Dec = Decimal<Prec, RoundPolicy>;
  UASSERT(((before >= 0) && (after >= 0)) || ((before <= 0) && (after <= 0)));

  int64_t result{};
  if (__builtin_mul_overflow(before, Dec::kDecimalFactor, &result) ||
      __builtin_add_overflow(result, after, &result)) {
    throw OutOfBoundsError();
  }

  return Dec::FromUnbiased(result);
}

// FromUnpacked<Decimal<4>>(12, 34, 3) -> 12.034
template <int Prec, typename RoundPolicy>
constexpr Decimal<Prec, RoundPolicy> FromUnpacked(int64_t before, int64_t after,
                                                  int original_precision) {
  UASSERT(((before >= 0) && (after >= 0)) || ((before <= 0) && (after <= 0)));
  UASSERT(after > -Pow10(original_precision) &&
          after < Pow10(original_precision));

  if (original_precision <= Prec) {
    // direct mode
    const int missing_digits = Prec - original_precision;
    const int64_t factor = Pow10(missing_digits);
    return FromUnpacked<Prec, RoundPolicy>(before, after * factor);
  } else {
    // rounding mode
    const int extra_digits = original_precision - Prec;
    const int64_t factor = Pow10(extra_digits);
    // note: if rounded up, rounded_after may represent a "fractional part"
    // greater than 1.0, which is ok
    const int64_t rounded_after = Div<RoundPolicy>(after, factor);
    return FromUnpacked<Prec, RoundPolicy>(before, rounded_after);
  }
}

struct UnpackedDecimal {
  int64_t before;
  int64_t after;
};

// AsUnpacked(Decimal<4>{"3.14"}) -> {3, 1400}
// AsUnpacked(Decimal<4>{"-3.14"}) -> {-3, -1400}
// AsUnpacked(Decimal<4>{"-0.14"}) -> {0, -1400}
template <int Prec, typename RoundPolicy>
constexpr UnpackedDecimal AsUnpacked(Decimal<Prec, RoundPolicy> dec) {
  using Dec = Decimal<Prec, RoundPolicy>;
  return {dec.AsUnbiased() / Dec::kDecimalFactor,
          dec.AsUnbiased() % Dec::kDecimalFactor};
}

// AsUnpacked(Decimal<4>{"3.14"}, 5) -> {3, 14000}
// AsUnpacked(Decimal<4>{"-3.14"}, 6) -> {-3, -140000}
// AsUnpacked(Decimal<4>{"-0.14"}, 1) -> {0, -1}
template <int Prec, typename RoundPolicy>
UnpackedDecimal AsUnpacked(Decimal<Prec, RoundPolicy> dec, int new_prec) {
  if (new_prec == Prec) {
    return AsUnpacked(dec);
  }
  int64_t result{};
  if (new_prec > Prec) {
    if (__builtin_mul_overflow(dec.AsUnbiased(), Pow10(new_prec - Prec),
                               &result)) {
      throw OutOfBoundsError();
    }
  } else {
    result = impl::Div<RoundPolicy>(dec.AsUnbiased(), Pow10(Prec - new_prec));
  }
  const auto dec_factor = Pow10(new_prec);
  return {result / dec_factor, result % dec_factor};
}

template <typename CharT>
constexpr bool IsSpace(CharT c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v';
}

template <typename CharT, typename Traits>
class StringCharSequence {
 public:
  explicit constexpr StringCharSequence(
      std::basic_string_view<CharT, Traits> sv)
      : current_(sv.begin()), end_(sv.end()) {}

  // on sequence end, returns '\0'
  constexpr CharT Get() { return current_ == end_ ? CharT{'\0'} : *current_++; }

  constexpr void Unget() { --current_; }

 private:
  typename std::basic_string_view<CharT, Traits>::iterator current_;
  typename std::basic_string_view<CharT, Traits>::iterator end_;
};

template <typename CharT, typename Traits>
class StreamCharSequence {
 public:
  explicit StreamCharSequence(std::basic_istream<CharT, Traits>& in)
      : in_(&in) {}

  // on sequence end, returns '\0'
  CharT Get() {
    constexpr CharT kEof =
        std::basic_istream<CharT, Traits>::traits_type::eof();
    if (!in_->good()) {
      return CharT{'\0'};
    }
    const CharT c = in_->peek();
    if (c == kEof) {
      return CharT{'\0'};
    }
    in_->ignore();
    return c;
  }

  void Unget() { in_->unget(); }

 private:
  std::basic_istream<CharT, Traits>* in_;
};

enum class ParseOptions {
  kNone = 0,

  /// Allow space characters in the beginning or in the end
  /// " 42  "
  kAllowSpaces = 1 << 0,

  /// Allow any trailing characters
  /// "42ABC"
  kAllowTrailingJunk = 1 << 1,

  /// Allow leading or trailing dot
  /// "42.", ".42"
  kAllowBoundaryDot = 1 << 2,

  /// Allow decimal digits beyond Prec, round according to RoundPolicy
  /// "0.123456" -> "0.1234" or "0.1235"
  kAllowRounding = 1 << 3
};

enum class ParseErrorCode : uint8_t {
  /// An unexpected character has been met
  kWrongChar,

  /// No digits before or after dot
  kNoDigits,

  /// The integral part does not fit in a Decimal
  kOverflow,

  /// The string contains leading spaces, while disallowed by options
  kSpace,

  /// The string contains trailing junk, while disallowed by options
  kTrailingJunk,

  /// On inputs like "42." or ".42" if disallowed by options
  kBoundaryDot,

  /// When there are more decimal digits than in any Decimal and rounding is
  /// disallowed by options
  kRounding,
};

struct ParseUnpackedResult {
  int64_t before{0};
  int64_t after{0};
  uint8_t decimal_digits{0};
  bool is_negative{false};
  std::optional<ParseErrorCode> error;
  uint32_t error_position{-1U};
};

enum class ParseState {
  /// Before reading any part of the Decimal
  kSign,

  /// After reading a sign
  kBeforeFirstDig,

  /// Only leading zeros (at least one) have been met
  kLeadingZeros,

  /// At least one digit before dot has been met
  kBeforeDec,

  /// Reading fractional digits
  kAfterDec,

  /// Reading and rounding extra fractional digits
  kIgnoringAfterDec,

  /// A character unrelated to the Decimal has been met
  kEnd
};

/// Extract values from a CharSequence ready to be packed to Decimal
template <typename CharSequence>
[[nodiscard]] constexpr ParseUnpackedResult ParseUnpacked(
    CharSequence input, utils::Flags<ParseOptions> options) {
  constexpr char dec_point = '.';

  int64_t before = 0;
  int64_t after = 0;
  bool is_negative = false;

  ptrdiff_t position = -1;
  auto state = ParseState::kSign;
  std::optional<ParseErrorCode> error;
  int before_digit_count = 0;
  uint8_t after_digit_count = 0;

  while (state != ParseState::kEnd) {
    const auto c = input.Get();
    if (c == '\0') break;
    if (!error) ++position;

    switch (state) {
      case ParseState::kSign:
        if (c == '-') {
          is_negative = true;
          state = ParseState::kBeforeFirstDig;
        } else if (c == '+') {
          state = ParseState::kBeforeFirstDig;
        } else if (c == '0') {
          state = ParseState::kLeadingZeros;
          before_digit_count = 1;
        } else if ((c >= '1') && (c <= '9')) {
          state = ParseState::kBeforeDec;
          before = static_cast<int>(c - '0');
          before_digit_count = 1;
        } else if (c == dec_point) {
          if (!(options & ParseOptions::kAllowBoundaryDot) && !error) {
            error = ParseErrorCode::kBoundaryDot;  // keep reading digits
          }
          state = ParseState::kAfterDec;
        } else if (IsSpace(c)) {
          if (!(options & ParseOptions::kAllowSpaces)) {
            state = ParseState::kEnd;
            error = ParseErrorCode::kSpace;
          }
        } else {
          state = ParseState::kEnd;
          error = ParseErrorCode::kWrongChar;
        }
        break;
      case ParseState::kBeforeFirstDig:
        if (c == '0') {
          state = ParseState::kLeadingZeros;
          before_digit_count = 1;
        } else if ((c >= '1') && (c <= '9')) {
          state = ParseState::kBeforeDec;
          before = static_cast<int>(c - '0');
          before_digit_count = 1;
        } else if (c == dec_point) {
          if (!(options & ParseOptions::kAllowBoundaryDot) && !error) {
            error = ParseErrorCode::kBoundaryDot;  // keep reading digits
          }
          state = ParseState::kAfterDec;
        } else {
          state = ParseState::kEnd;
          error = ParseErrorCode::kWrongChar;
        }
        break;
      case ParseState::kLeadingZeros:
        if (c == '0') {
          // skip
        } else if ((c >= '1') && (c <= '9')) {
          state = ParseState::kBeforeDec;
          before = static_cast<int>(c - '0');
        } else if (c == dec_point) {
          state = ParseState::kAfterDec;
        } else {
          state = ParseState::kEnd;
        }
        break;
      case ParseState::kBeforeDec:
        if ((c >= '0') && (c <= '9')) {
          if (before_digit_count < kMaxDecimalDigits) {
            before = 10 * before + static_cast<int>(c - '0');
            before_digit_count++;
          } else if (!error) {
            error = ParseErrorCode::kOverflow;  // keep reading digits
          }
        } else if (c == dec_point) {
          state = ParseState::kAfterDec;
        } else {
          state = ParseState::kEnd;
        }
        break;
      case ParseState::kAfterDec:
        if ((c >= '0') && (c <= '9')) {
          if (after_digit_count < kMaxDecimalDigits) {
            after = 10 * after + static_cast<int>(c - '0');
            after_digit_count++;
          } else {
            if (!(options & ParseOptions::kAllowRounding) && !error) {
              error = ParseErrorCode::kRounding;  // keep reading digits
            }
            state = ParseState::kIgnoringAfterDec;
            if (c >= '5') {
              // round half up
              after++;
            }
          }
        } else {
          if (!(options & ParseOptions::kAllowBoundaryDot) &&
              after_digit_count == 0 && !error) {
            error = ParseErrorCode::kBoundaryDot;
          }
          state = ParseState::kEnd;
        }
        break;
      case ParseState::kIgnoringAfterDec:
        if ((c >= '0') && (c <= '9')) {
          // skip
        } else {
          state = ParseState::kEnd;
        }
        break;
      case ParseState::kEnd:
        UASSERT(false);
        break;
    }  // switch state
  }    // while has more chars & not end

  if (state == ParseState::kEnd) {
    input.Unget();

    if (!error && !(options & ParseOptions::kAllowTrailingJunk)) {
      if (!(options & ParseOptions::kAllowSpaces)) {
        error = ParseErrorCode::kSpace;
      }
      --position;

      while (true) {
        const auto c = input.Get();
        if (c == '\0') break;
        ++position;
        if (!IsSpace(c)) {
          error = ParseErrorCode::kTrailingJunk;
          input.Unget();
          break;
        }
      }
    }
  }

  if (!error && before_digit_count == 0 && after_digit_count == 0) {
    error = ParseErrorCode::kNoDigits;
  }

  if (!error && state == ParseState::kAfterDec &&
      !(options & ParseOptions::kAllowBoundaryDot) && after_digit_count == 0) {
    error = ParseErrorCode::kBoundaryDot;
  }

  return {before,      after, after_digit_count,
          is_negative, error, static_cast<uint32_t>(position)};
}

template <int Prec, typename RoundPolicy>
struct ParseResult {
  Decimal<Prec, RoundPolicy> decimal;
  std::optional<ParseErrorCode> error;
  uint32_t error_position{-1U};
};

/// Parse Decimal from a CharSequence
template <int Prec, typename RoundPolicy, typename CharSequence>
[[nodiscard]] constexpr ParseResult<Prec, RoundPolicy> Parse(
    CharSequence input, utils::Flags<ParseOptions> options) {
  ParseUnpackedResult parsed = ParseUnpacked(input, options);

  if (parsed.error) {
    return {{}, parsed.error, parsed.error_position};
  }

  if (parsed.before >= kMaxInt64 / kPow10<Prec>) {
    return {{}, ParseErrorCode::kOverflow, 0};
  }

  if (!(options & ParseOptions::kAllowRounding) &&
      parsed.decimal_digits > Prec) {
    return {{}, ParseErrorCode::kRounding, 0};
  }

  if (parsed.is_negative) {
    parsed.before = -parsed.before;
    parsed.after = -parsed.after;
  }

  return {FromUnpacked<Prec, RoundPolicy>(parsed.before, parsed.after,
                                          parsed.decimal_digits),
          {},
          0};
}

std::string GetErrorMessage(std::string_view source, std::string_view path,
                            size_t position, ParseErrorCode reason);

/// removes the zeros on the right in after
/// saves the updated precision in after_precision
void TrimTrailingZeros(int64_t& after, int& after_precision);

std::string ToString(int64_t before, int64_t after, int precision,
                     const FormatOptions& format_options);

}  // namespace impl

template <int Prec, typename RoundPolicy>
constexpr Decimal<Prec, RoundPolicy>::Decimal(std::string_view value) {
  const auto result = impl::Parse<Prec, RoundPolicy>(
      impl::StringCharSequence(value), impl::ParseOptions::kNone);

  if (result.error) {
    throw ParseError(impl::GetErrorMessage(
        value, "<string>", result.error_position, *result.error));
  }
  *this = result.decimal;
}

template <int Prec, typename RoundPolicy>
constexpr Decimal<Prec, RoundPolicy>
Decimal<Prec, RoundPolicy>::FromStringPermissive(std::string_view input) {
  const auto result = impl::Parse<Prec, RoundPolicy>(
      impl::StringCharSequence(input),
      {impl::ParseOptions::kAllowSpaces, impl::ParseOptions::kAllowBoundaryDot,
       impl::ParseOptions::kAllowRounding});

  if (result.error) {
    throw ParseError(impl::GetErrorMessage(
        input, "<string>", result.error_position, *result.error));
  }
  return result.decimal;
}

/// @brief Converts Decimal to a string
///
/// Usage example:
///
///     ToString(decimal64::Decimal<4>{"1.5"}) -> 1.5
///
/// @see ToStringTrailingZeros
/// @see ToStringFixed
template <int Prec, typename RoundPolicy>
std::string ToString(Decimal<Prec, RoundPolicy> dec) {
  return fmt::to_string(dec);
}

/// @brief Converts Decimal to a string
///
/// Usage example:
///
///     ToString(decimal64::Decimal<4>{"-1234.1234"},
///              {"||", "**", "\1", "<>", {}, true}))   -> "<>1**2**3**4||1234"
///     ToString(decimal64::Decimal<4>{"-1234.1234"},
///              {",", " ", "\3", "-", 6, true}))       -> "-1 234,123400"
///
/// @see ToStringTrailingZeros
/// @see ToStringFixed
template <int Prec, typename RoundPolicy>
std::string ToString(const Decimal<Prec, RoundPolicy>& dec,
                     const FormatOptions& format_options) {
  auto precision = format_options.precision.value_or(Prec);
  if (!format_options.is_fixed) {
    precision = std::min(precision, Prec);
  }
  auto [before, after] = impl::AsUnpacked(dec, precision);
  return impl::ToString(before, after, precision, format_options);
}

/// @brief Converts Decimal to a string, writing exactly `Prec` decimal digits
///
/// Usage example:
///
///     ToStringTrailingZeros(decimal64::Decimal<4>{"1.5"}) -> 1.5000
///
/// @see ToString
/// @see ToStringFixed
template <int Prec, typename RoundPolicy>
std::string ToStringTrailingZeros(Decimal<Prec, RoundPolicy> dec) {
  return fmt::format(FMT_COMPILE("{:f}"), dec);
}

/// @brief Converts Decimal to a string with exactly `NewPrec` decimal digits
///
/// Usage example:
///
///     ToStringFixed<3>(decimal64::Decimal<4>{"1.5"}) -> 1.500
///
/// @see ToString
/// @see ToStringTrailingZeros
template <int NewPrec, int Prec, typename RoundPolicy>
std::string ToStringFixed(Decimal<Prec, RoundPolicy> dec) {
  return ToStringTrailingZeros(
      decimal64::decimal_cast<Decimal<NewPrec, RoundPolicy>>(dec));
}

/// @brief Parses a `Decimal` from the `istream`
///
/// Acts like the `Decimal(str)` constructor, except that it allows junk that
/// immediately follows the number. Sets the stream's fail bit on failure.
///
/// Usage example:
///
///     if (os >> dec) {
///       // success
///     } else {
///       // failure
///     }
///
/// @see Decimal::Decimal(std::string_view)
template <typename CharT, typename Traits, int Prec, typename RoundPolicy>
std::basic_istream<CharT, Traits>& operator>>(
    std::basic_istream<CharT, Traits>& is, Decimal<Prec, RoundPolicy>& d) {
  if (is.flags() & std::ios_base::skipws) {
    std::ws(is);
  }
  const auto result = impl::Parse<Prec, RoundPolicy>(
      impl::StreamCharSequence(is), {impl::ParseOptions::kAllowTrailingJunk});

  if (result.error) {
    is.setstate(std::ios_base::failbit);
  } else {
    d = result.decimal;
  }
  return is;
}

/// @brief Writes the `Decimal` to the `ostream`
/// @see ToString
template <typename CharT, typename Traits, int Prec, typename RoundPolicy>
std::basic_ostream<CharT, Traits>& operator<<(
    std::basic_ostream<CharT, Traits>& os,
    const Decimal<Prec, RoundPolicy>& d) {
  os << ToString(d);
  return os;
}

/// @brief Writes the `Decimal` to the logger
/// @see ToString
template <int Prec, typename RoundPolicy>
logging::LogHelper& operator<<(logging::LogHelper& lh,
                               const Decimal<Prec, RoundPolicy>& d) {
  lh << ToString(d);
  return lh;
}

/// @brief Parses the `Decimal` from the string
/// @see Decimal::Decimal(std::string_view)
template <int Prec, typename RoundPolicy, typename Value>
std::enable_if_t<formats::common::kIsFormatValue<Value>,
                 Decimal<Prec, RoundPolicy>>
Parse(const Value& value, formats::parse::To<Decimal<Prec, RoundPolicy>>) {
  const std::string input = value.template As<std::string>();

  const auto result = impl::Parse<Prec, RoundPolicy>(
      impl::StringCharSequence(std::string_view{input}),
      impl::ParseOptions::kNone);

  if (result.error) {
    throw ParseError(impl::GetErrorMessage(
        input, value.GetPath(), result.error_position, *result.error));
  }
  return result.decimal;
}

/// @brief Serializes the `Decimal` to string
/// @see ToString
template <int Prec, typename RoundPolicy, typename TargetType>
TargetType Serialize(const Decimal<Prec, RoundPolicy>& object,
                     formats::serialize::To<TargetType>) {
  return typename TargetType::Builder(ToString(object)).ExtractValue();
}

/// @brief Writes the `Decimal` to stream
/// @see ToString
template <int Prec, typename RoundPolicy, typename StringBuilder>
void WriteToStream(const Decimal<Prec, RoundPolicy>& object,
                   StringBuilder& sw) {
  WriteToStream(ToString(object), sw);
}

}  // namespace decimal64

USERVER_NAMESPACE_END

/// std::hash support
template <int Prec, typename RoundPolicy>
struct std::hash<USERVER_NAMESPACE::decimal64::Decimal<Prec, RoundPolicy>> {
  using Decimal = USERVER_NAMESPACE::decimal64::Decimal<Prec, RoundPolicy>;

  std::size_t operator()(const Decimal& v) const noexcept {
    return std::hash<int64_t>{}(v.AsUnbiased());
  }
};

/// @brief fmt support
///
/// Spec format:
/// - {} trims any trailing zeros;
/// - {:f} writes exactly `Prec` decimal digits, including trailing zeros
///   if needed.
/// - {:.N} writes exactly `N` decimal digits, including trailing zeros
///   if needed.
template <int Prec, typename RoundPolicy, typename Char>
class fmt::formatter<USERVER_NAMESPACE::decimal64::Decimal<Prec, RoundPolicy>,
                     Char> {
 public:
  constexpr auto parse(fmt::basic_format_parse_context<Char>& ctx) {
    const auto* it = ctx.begin();
    const auto* end = ctx.end();

    if (it != end && *it == '.') {
      remove_trailing_zeros_ = false;
      custom_precision_ = 0;
      ++it;
      while (it != end && *it >= '0' && *it <= '9') {
        *custom_precision_ = *custom_precision_ * 10 + (*it - '0');
        ++it;
      }
    }

    if (!custom_precision_ && it != end && *it == 'f') {
      remove_trailing_zeros_ = false;
      ++it;
    }

    if (it != end && *it != '}') {
      throw format_error("invalid format");
    }

    return it;
  }

  template <typename FormatContext>
  auto format(
      const USERVER_NAMESPACE::decimal64::Decimal<Prec, RoundPolicy>& dec,
      FormatContext& ctx) const {
    int after_digits = custom_precision_.value_or(Prec);
    auto [before, after] =
        USERVER_NAMESPACE::decimal64::impl::AsUnpacked(dec, after_digits);
    if (remove_trailing_zeros_) {
      USERVER_NAMESPACE::decimal64::impl::TrimTrailingZeros(after,
                                                            after_digits);
    }

    if (after_digits > 0) {
      if (dec.Sign() == -1) {
        return fmt::format_to(ctx.out(), FMT_COMPILE("-{}.{:0{}}"), -before,
                              -after, after_digits);
      } else {
        return fmt::format_to(ctx.out(), FMT_COMPILE("{}.{:0{}}"), before,
                              after, after_digits);
      }
    } else {
      return fmt::format_to(ctx.out(), FMT_COMPILE("{}"), before);
    }
  }

 private:
  bool remove_trailing_zeros_ = true;
  std::optional<int> custom_precision_;
};
