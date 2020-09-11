#pragma once

/// @file utils/decimal64.hpp
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
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include <fmt/format.h>

#include <utils/assert.hpp>
#include <utils/meta.hpp>

namespace decimal64 {

namespace impl {

// 0 for strong typing (same precision required for
// both arguments),
// 1 for allowing to mix lower or equal precision types
// 2 for automatic rounding when different precision is mixed
inline constexpr int kTypeLevel = 2;

template <int PrecFrom, int PrecTo>
constexpr void CheckPrecCast() {
  static_assert(
      kTypeLevel >= 2 || (kTypeLevel >= 1 && PrecFrom <= PrecTo),
      "This implicit decimal cast is not allowed under current settings");
}

inline constexpr auto kMaxInt64 = std::numeric_limits<int64_t>::max();
inline constexpr auto kMinInt64 = std::numeric_limits<int64_t>::min();

template <typename T, typename R = void>
using EnableIfInt = std::enable_if_t<meta::kIsInteger<T>, R>;

template <int MaxExp>
constexpr std::array<int64_t, MaxExp + 1> PowSeries(int64_t base) {
  int64_t pow = 1;
  std::array<int64_t, MaxExp + 1> result{};
  for (int i = 0; i < MaxExp; ++i) {
    result[i] = pow;
    if (pow > kMaxInt64 / base) {
      throw std::runtime_error("Overflow");
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

constexpr bool IsMultOverflowPositive(int64_t value1, int64_t value2) {
  UASSERT(value1 > 0 && value2 > 0);
  return value1 > impl::kMaxInt64 / value2 && value2 > impl::kMaxInt64 / value1;
}

constexpr bool IsMultOverflow(int64_t value1, int64_t value2) {
  if (value1 == 0 || value2 == 0) {
    return false;
  }

  if ((value1 < 0) != (value2 < 0)) {  // different sign
    if (value1 == impl::kMinInt64) {
      return value2 > 1;
    } else if (value2 == impl::kMinInt64) {
      return value1 > 1;
    }
    if (value1 < 0) {
      return IsMultOverflowPositive(-value1, value2);
    }
    if (value2 < 0) {
      return IsMultOverflowPositive(value1, -value2);
    }
  } else if (value1 < 0 && value2 < 0) {
    if (value1 == impl::kMinInt64) {
      return value2 < -1;
    } else if (value2 == impl::kMinInt64) {
      return value1 < -1;
    }
    return IsMultOverflowPositive(-value1, -value2);
  }

  return IsMultOverflowPositive(value1, value2);
}

// result = (value1 * value2) / divisor
template <class RoundPolicy>
constexpr int64_t MultDiv(int64_t value1, int64_t value2, int64_t divisor) {
  // we don't check for division by zero, the caller should - the next line
  // will throw.
  const int64_t value1int = value1 / divisor;
  int64_t value1dec = value1 % divisor;
  const int64_t value2int = value2 / divisor;
  int64_t value2dec = value2 % divisor;

  int64_t result = value1 * value2int + value1int * value2dec;

  if (value1dec == 0 || value2dec == 0) {
    return result;
  }

  if (!IsMultOverflow(value1dec, value2dec)) {  // no overflow
    int64_t res_dec_part = value1dec * value2dec;
    if (!RoundPolicy::DivRounded(res_dec_part, res_dec_part, divisor)) {
      res_dec_part = 0;
    }
    result += res_dec_part;
    return result;
  }

  // reduce value1 & divisor
  {
    const int64_t c = std::gcd(value1dec, divisor);
    if (c != 1) {
      value1dec /= c;
      divisor /= c;
    }
  }

  // reduce value2 & divisor
  {
    const int64_t c = std::gcd(value2dec, divisor);
    if (c != 1) {
      value2dec /= c;
      divisor /= c;
    }
  }

  if (!IsMultOverflow(value1dec, value2dec)) {  // no overflow
    int64_t res_dec_part = value1dec * value2dec;
    if (RoundPolicy::DivRounded(res_dec_part, res_dec_part, divisor)) {
      result += res_dec_part;
      return result;
    }
  }

  // overflow can occur - use less precise version
  result += RoundPolicy::Round(static_cast<long double>(value1dec) *
                               static_cast<long double>(value2dec) /
                               static_cast<long double>(divisor));
  return result;
}

// Needed because std::abs is not constexpr
template <typename T>
constexpr T Abs(T value) {
  return value >= T(0) ? value : -value;
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

}  // namespace impl

constexpr int64_t Pow10(int exp) {
  if (exp < 0 || exp > impl::kMaxDecimalDigits) {
    throw std::runtime_error("Pow10: invalid power of 10");
  }
  return impl::kPowSeries10[static_cast<size_t>(exp)];
}

template <int Exp>
inline constexpr int64_t kPow10 = Pow10(Exp);

// no-rounding policy (decimal places stripped)
class NullRoundPolicy {
 public:
  template <class T>
  static constexpr int64_t Round(T value) {
    return static_cast<int64_t>(value);
  }

  [[nodiscard]] static constexpr bool DivRounded(int64_t& output, int64_t a,
                                                 int64_t b) {
    output = a / b;
    return true;
  }
};

// default rounding policy - arithmetic, to nearest integer
class DefRoundPolicy {
 public:
  // round floating point value and convert to int64_t
  template <class T>
  static constexpr int64_t Round(T value) {
    return static_cast<int64_t>(value + (value < 0 ? -0.5 : 0.5));
  }

  // calculate output = round(a / b), where output, a, b are int64_t
  [[nodiscard]] static constexpr bool DivRounded(int64_t& output, int64_t a,
                                                 int64_t b) {
    const int64_t divisor_corr = impl::Abs(b / 2);
    if (a >= 0) {
      if (impl::kMaxInt64 - a >= divisor_corr) {
        output = (a + divisor_corr) / b;
        return true;
      }
    } else {
      if (-(impl::kMinInt64 - a) >= divisor_corr) {
        output = (a - divisor_corr) / b;
        return true;
      }
    }

    output = 0;
    return false;
  }
};

class HalfDownRoundPolicy {
 public:
  template <class T>
  static constexpr int64_t Round(T value) {
    if (value >= 0.0) {
      const T decimals = value - impl::Floor(value);
      if (decimals > 0.5) {
        return impl::Ceil(value);
      } else {
        return impl::Floor(value);
      }
    } else {
      const T decimals = impl::Ceil(value) - value;
      if (decimals < 0.5) {
        return impl::Ceil(value);
      } else {
        return impl::Floor(value);
      }
    }
  }

  [[nodiscard]] static constexpr bool DivRounded(int64_t& output, int64_t a,
                                                 int64_t b) {
    int64_t divisor_corr = impl::Abs(b) / 2;
    int64_t remainder = impl::Abs(a) % impl::Abs(b);

    if (a >= 0) {
      if (impl::kMaxInt64 - a >= divisor_corr) {
        if (remainder > divisor_corr) {
          output = (a + divisor_corr) / b;
        } else {
          output = a / b;
        }
        return true;
      }
    } else {
      if (-(impl::kMinInt64 - a) >= divisor_corr) {
        output = (a - divisor_corr) / b;
        return true;
      }
    }

    output = 0;
    return false;
  }
};

class HalfUpRoundPolicy {
 public:
  template <class T>
  static constexpr int64_t Round(T value) {
    if (value >= 0.0) {
      const T decimals = value - impl::Floor(value);
      if (decimals >= 0.5) {
        return impl::Ceil(value);
      } else {
        return impl::Floor(value);
      }
    } else {
      const T decimals = impl::Ceil(value) - value;
      if (decimals <= 0.5) {
        return impl::Ceil(value);
      } else {
        return impl::Floor(value);
      }
    }
  }

  [[nodiscard]] static constexpr bool DivRounded(int64_t& output, int64_t a,
                                                 int64_t b) {
    const int64_t divisor_corr = impl::Abs(b) / 2;
    const int64_t remainder = impl::Abs(a) % impl::Abs(b);

    if (a >= 0) {
      if (impl::kMaxInt64 - a >= divisor_corr) {
        if (remainder >= divisor_corr) {
          output = (a + divisor_corr) / b;
        } else {
          output = a / b;
        }
        return true;
      }
    } else {
      if (-(impl::kMinInt64 - a) >= divisor_corr) {
        if (remainder < divisor_corr) {
          output = (a - remainder) / b;
        } else if (remainder == divisor_corr) {
          output = (a + divisor_corr) / b;
        } else {
          output = (a + remainder - impl::Abs(b)) / b;
        }
        return true;
      }
    }

    output = 0;
    return false;
  }
};

// bankers' rounding
class HalfEvenRoundPolicy {
 public:
  template <class T>
  static constexpr int64_t Round(T value) {
    if (value >= 0.0) {
      const T decimals = value - impl::Floor(value);
      if (decimals > 0.5) {
        return impl::Ceil(value);
      } else if (decimals < 0.5) {
        return impl::Floor(value);
      } else {
        const bool is_even = static_cast<int64_t>(value - decimals) % 2 == 0;
        if (is_even) {
          return impl::Floor(value);
        } else {
          return impl::Ceil(value);
        }
      }
    } else {
      const T decimals = std::abs(value + impl::Floor(std::abs(value)));
      if (decimals > 0.5) {
        return impl::Floor(value);
      } else if (decimals < 0.5) {
        return impl::Ceil(value);
      } else {
        const bool is_even = static_cast<int64_t>(value + decimals) % 2 == 0;
        if (is_even) {
          return impl::Ceil(value);
        } else {
          return impl::Floor(value);
        }
      }
    }
  }

  [[nodiscard]] static constexpr bool DivRounded(int64_t& output, int64_t a,
                                                 int64_t b) {
    const int64_t divisor_div2 = impl::Abs(b) / 2;
    const int64_t remainder = impl::Abs(a) % impl::Abs(b);

    if (remainder == 0) {
      output = a / b;
    } else {
      if (a >= 0) {
        if (remainder > divisor_div2) {
          output = (a - remainder + impl::Abs(b)) / b;
        } else if (remainder < divisor_div2) {
          output = (a - remainder) / b;
        } else {
          const bool is_even = impl::Abs(a / b) % 2 == 0;
          if (is_even) {
            output = a / b;
          } else {
            output = (a - remainder + impl::Abs(b)) / b;
          }
        }
      } else {
        // negative value
        if (remainder > divisor_div2) {
          output = (a + remainder - impl::Abs(b)) / b;
        } else if (remainder < divisor_div2) {
          output = (a + remainder) / b;
        } else {
          const bool is_even = impl::Abs(a / b) % 2 == 0;
          if (is_even) {
            output = a / b;
          } else {
            output = (a + remainder - impl::Abs(b)) / b;
          }
        }
      }
    }

    return true;
  }
};

// round towards +infinity
class CeilingRoundPolicy {
 public:
  template <class T>
  static constexpr int64_t Round(T value) {
    return impl::Ceil(value);
  }

  [[nodiscard]] static constexpr bool DivRounded(int64_t& output, int64_t a,
                                                 int64_t b) {
    const int64_t remainder = impl::Abs(a) % impl::Abs(b);
    if (remainder == 0) {
      output = a / b;
    } else {
      if (a >= 0) {
        output = (a + impl::Abs(b)) / b;
      } else {
        output = a / b;
      }
    }
    return true;
  }
};

// round towards -infinity
class FloorRoundPolicy {
 public:
  template <class T>
  static constexpr int64_t Round(T value) {
    return impl::Floor(value);
  }

  [[nodiscard]] static constexpr bool DivRounded(int64_t& output, int64_t a,
                                                 int64_t b) {
    const int64_t remainder = impl::Abs(a) % impl::Abs(b);
    if (remainder == 0) {
      output = a / b;
    } else {
      if (a >= 0) {
        output = (a - remainder) / b;
      } else {
        output = (a + remainder - impl::Abs(b)) / b;
      }
    }
    return true;
  }
};

// round towards zero = truncate
class RoundDownRoundPolicy : public NullRoundPolicy {};

// round away from zero
class RoundUpRoundPolicy {
 public:
  template <class T>
  static constexpr int64_t Round(T value) {
    if (value >= 0.0) {
      return impl::Ceil(value);
    } else {
      return impl::Floor(value);
    }
  }

  [[nodiscard]] static constexpr bool DivRounded(int64_t& output, int64_t a,
                                                 int64_t b) {
    const int64_t remainder = impl::Abs(a) % impl::Abs(b);
    if (remainder == 0) {
      output = a / b;
    } else {
      if (a >= 0) {
        output = (a + impl::Abs(b)) / b;
      } else {
        output = (a - impl::Abs(b)) / b;
      }
    }
    return true;
  }
};

struct UnpackedDecimal {
  int64_t before;
  int64_t after;
};

struct FloatingPointDecimal {
  int64_t mantissa;
  int exponent;
};

/// Decimal value type. Use for capital calculations.
/// Note: maximum handled value is: +9,223,372,036,854,775,807 (divided by prec)
///
/// Sample usage:
///   using namespace decimal64;
///   decimal<2> value(143125);
///   value = value / decimal_cast<2>(333);
///   cout << "Result is: " << value << endl;
template <int Prec, class RoundPolicy_ = DefRoundPolicy>
class Decimal {
 public:
  static constexpr int kDecimalPoints = Prec;
  using RoundPolicy = RoundPolicy_;

  static constexpr int64_t kDecimalFactor = kPow10<Prec>;

  constexpr Decimal() noexcept : value_(0) {}

  template <typename T, typename = impl::EnableIfInt<T>>
  constexpr explicit Decimal(T value) : Decimal(FromInteger(value)) {}
  constexpr explicit Decimal(long double value)
      : Decimal(FromFloatingPoint(value)) {}
  constexpr explicit Decimal(double value)
      : Decimal(FromFloatingPoint(value)) {}
  constexpr explicit Decimal(float value) : Decimal(FromFloatingPoint(value)) {}

  explicit Decimal(std::string_view value);

  static constexpr Decimal FromUnbiased(int64_t value) noexcept {
    Decimal result;
    result.value_ = value;
    return result;
  }

  /// Combines two parts (before and after decimal point) into decimal value.
  /// Both input values have to have the same sign for correct results.
  /// Does not perform any rounding or input validation - after must be
  /// less than 10^prec.
  /// \param[in] before value before decimal point
  /// \param[in] after value after decimal point multiplied by 10^prec
  static constexpr Decimal FromUnpacked(int64_t before, int64_t after) {
    if constexpr (Prec > 0) {
      return FromUnbiased(before * kDecimalFactor + after);
    } else {
      return FromUnbiased(before * kDecimalFactor);
    }
  }

  /// Combines two parts (before and after decimal point) into decimal value.
  /// Both input values have to have the same sign for correct results.
  /// Does not perform any rounding or input validation - after must be
  /// less than 10^prec.
  /// \param[in] before value before decimal point
  /// \param[in] after value after decimal point multiplied by 10^prec
  /// \param[in] original_precision the number of decimal digits in after
  static constexpr Decimal FromUnpacked(int64_t before, int64_t after,
                                        int original_precision) {
    if (original_precision <= Prec) {
      // direct mode
      const int missing_digits = Prec - original_precision;
      const int64_t factor = Pow10(missing_digits);
      return FromUnpacked(before, after * factor);
    } else {
      // rounding mode
      const int extra_digits = original_precision - Prec;
      const int64_t factor = Pow10(extra_digits);
      int64_t rounded_after{};
      if (!RoundPolicy::DivRounded(rounded_after, after, factor)) {
        return Decimal(0);
      }
      return FromUnpacked(before, rounded_after);
    }
  }

  static constexpr Decimal FromBiased(int64_t original_unbiased,
                                      int original_precision) {
    const int exponent_for_pack = Prec - original_precision;

    if (exponent_for_pack >= 0) {
      return FromUnbiased(original_unbiased * Pow10(exponent_for_pack));
    } else {
      int64_t new_value{};

      if (!RoundPolicy::DivRounded(new_value, original_unbiased,
                                   Pow10(-exponent_for_pack))) {
        new_value = 0;
      }

      return FromUnbiased(new_value);
    }
  }

  static constexpr Decimal FromFraction(int64_t nom, int64_t den) {
    return FromUnbiased(impl::MultDiv<RoundPolicy>(kDecimalFactor, nom, den));
  }

  template <int Prec2>
  Decimal& operator=(Decimal<Prec2, RoundPolicy> rhs) {
    impl::CheckPrecCast<Prec2, Prec>();
    if constexpr (Prec2 <= Prec) {
      value_ = rhs.value_ * kPow10<Prec - Prec2>;
    } else {
      if (!RoundPolicy::DivRounded(value_, rhs.value_, kPow10<Prec2 - Prec>)) {
        value_ = 0;
      }
    }
    return *this;
  }

  template <typename T, typename = impl::EnableIfInt<T>>
  constexpr Decimal& operator=(T rhs) {
    return *this = Decimal{rhs};
  }

  constexpr bool operator==(Decimal rhs) const { return value_ == rhs.value_; }

  constexpr bool operator!=(Decimal rhs) const { return value_ != rhs.value_; }

  constexpr bool operator<(Decimal rhs) const { return value_ < rhs.value_; }

  constexpr bool operator<=(Decimal rhs) const { return value_ <= rhs.value_; }

  constexpr bool operator>(Decimal rhs) const { return value_ > rhs.value_; }

  constexpr bool operator>=(Decimal rhs) const { return value_ >= rhs.value_; }

  constexpr Decimal operator+(Decimal rhs) const {
    Decimal result = *this;
    result.value_ += rhs.value_;
    return result;
  }

  template <int Prec2>
  constexpr Decimal operator+(Decimal<Prec2, RoundPolicy> rhs) const {
    Decimal result = *this;
    result += rhs;
    return result;
  }

  constexpr Decimal& operator+=(Decimal rhs) {
    value_ += rhs.value_;
    return *this;
  }

  template <int Prec2>
  constexpr Decimal& operator+=(Decimal<Prec2, RoundPolicy> rhs) {
    impl::CheckPrecCast<Prec2, Prec>();
    if constexpr (Prec2 <= Prec) {
      value_ += rhs.value_ * kPow10<Prec - Prec2>;
    } else {
      int64_t val{};
      if (!RoundPolicy::DivRounded(val, rhs.value_, kPow10<Prec2 - Prec>)) {
        val = 0;
      }
      value_ += val;
    }
    return *this;
  }

  constexpr Decimal operator+() const { return *this; }

  constexpr Decimal operator-() const {
    Decimal result = *this;
    result.value_ = -result.value_;
    return result;
  }

  constexpr Decimal operator-(Decimal rhs) const {
    Decimal result = *this;
    result.value_ -= rhs.value_;
    return result;
  }

  template <int Prec2>
  constexpr Decimal operator-(Decimal<Prec2, RoundPolicy> rhs) const {
    Decimal result = *this;
    result -= rhs;
    return result;
  }

  constexpr Decimal& operator-=(Decimal rhs) {
    value_ -= rhs.value_;
    return *this;
  }

  template <int Prec2>
  constexpr Decimal& operator-=(Decimal<Prec2, RoundPolicy> rhs) {
    impl::CheckPrecCast<Prec2, Prec>();
    if (Prec2 <= Prec) {
      value_ -= rhs.value_ * kPow10<Prec - Prec2>;
    } else {
      int64_t val{};
      if (!RoundPolicy::DivRounded(val, rhs.value_, kPow10<Prec2 - Prec>)) {
        val = 0;
      }
      value_ -= val;
    }
    return *this;
  }

  template <typename T, typename = impl::EnableIfInt<T>>
  constexpr Decimal operator*(T rhs) const {
    Decimal result = *this;
    result.value_ *= rhs;
    return result;
  }

  constexpr Decimal operator*(Decimal rhs) const {
    Decimal result = *this;
    result *= rhs;
    return result;
  }

  template <int Prec2>
  constexpr Decimal operator*(Decimal<Prec2, RoundPolicy> rhs) const {
    Decimal result = *this;
    result *= rhs;
    return result;
  }

  template <typename T, typename = impl::EnableIfInt<T>>
  constexpr Decimal& operator*=(T rhs) {
    value_ *= rhs;
    return *this;
  }

  constexpr Decimal& operator*=(Decimal rhs) {
    value_ = impl::MultDiv<RoundPolicy>(value_, rhs.value_, kPow10<Prec>);
    return *this;
  }

  template <int Prec2>
  constexpr Decimal& operator*=(Decimal<Prec2, RoundPolicy> rhs) {
    impl::CheckPrecCast<Prec2, Prec>();
    value_ = impl::MultDiv<RoundPolicy>(value_, rhs.value_, kPow10<Prec2>);
    return *this;
  }

  template <typename T, typename = impl::EnableIfInt<T>>
  constexpr Decimal operator/(T rhs) const {
    Decimal result = *this;
    result /= rhs;
    return result;
  }

  constexpr Decimal operator/(Decimal rhs) const {
    Decimal result = *this;
    result /= rhs;
    return result;
  }

  template <int Prec2>
  constexpr Decimal operator/(Decimal<Prec2, RoundPolicy> rhs) const {
    Decimal result = *this;
    result /= rhs;
    return result;
  }

  template <typename T, typename = impl::EnableIfInt<T>>
  constexpr Decimal& operator/=(T rhs) {
    if (!RoundPolicy::DivRounded(value_, value_, rhs)) {
      value_ = impl::MultDiv<RoundPolicy>(value_, 1, rhs);
    }
    return *this;
  }

  constexpr Decimal& operator/=(Decimal rhs) {
    value_ = impl::MultDiv<RoundPolicy>(value_, kPow10<Prec>, rhs.value_);
    return *this;
  }

  template <int Prec2>
  constexpr Decimal& operator/=(Decimal<Prec2, RoundPolicy> rhs) {
    impl::CheckPrecCast<Prec2, Prec>();
    value_ = impl::MultDiv<RoundPolicy>(value_, kPow10<Prec2>, rhs.value_);
    return *this;
  }

  /// Returns integer indicating sign of value
  /// -1 if value is < 0
  /// +1 if value is > 0
  /// 0  if value is 0
  constexpr int Sign() const {
    return (value_ > 0) ? 1 : ((value_ < 0) ? -1 : 0);
  }

  constexpr Decimal Abs() const { return FromUnbiased(impl::Abs(value_)); }

  /// returns value rounded to integer using active rounding policy
  constexpr int64_t ToInteger() const {
    int64_t result{};
    if (!RoundPolicy::DivRounded(result, value_, kDecimalFactor)) {
      result = 0;
    }
    return result;
  }

  constexpr double ToDouble() const {
    return static_cast<double>(value_) / kDecimalFactor;
  }

  constexpr long double ToLongDouble() const {
    return static_cast<long double>(value_) / kDecimalFactor;
  }

  /// returns integer value = real_value * (10 ^ precision)
  /// use to load/store Decimal value in external memory
  constexpr int64_t AsUnbiased() const { return value_; }

  /// Returns two parts: before and after decimal point
  /// For negative values both numbers are negative or zero.
  constexpr UnpackedDecimal AsUnpacked() const {
    return {value_ / kDecimalFactor, value_ % kDecimalFactor};
  }

 private:
  template <typename T>
  static constexpr Decimal FromInteger(T value) {
    return FromUnbiased(static_cast<int64_t>(value) * kDecimalFactor);
  }

  template <typename T>
  static constexpr Decimal FromFloatingPoint(T value) {
    const auto int_part = impl::Floor(value);
    const T frac_part = value - int_part;
    return FromUnpacked(
        int_part, RoundPolicy::Round(static_cast<long double>(kDecimalFactor) *
                                     frac_part));
  }

  int64_t value_;
};

namespace impl {

template <typename T>
struct IsDecimal : std::false_type {};

template <int Prec, typename RoundPolicy>
struct IsDecimal<Decimal<Prec, RoundPolicy>> : std::true_type {};

}  // namespace impl

/// Example of use:
///   c = decimal64::decimal_cast<6>(a * b);
template <int Prec, int OldPrec, class Round>
constexpr Decimal<Prec, Round> decimal_cast(Decimal<OldPrec, Round> arg) {
  return Decimal<Prec, Round>::FromBiased(arg.AsUnbiased(), OldPrec);
}

template <int Prec, typename Round, int OldPrec, typename OldRound>
constexpr Decimal<Prec, Round> decimal_cast(Decimal<OldPrec, OldRound> arg) {
  return Decimal<Prec, Round>::FromBiased(arg.AsUnbiased(), OldPrec);
}

template <int Prec, typename T>
constexpr impl::EnableIfInt<T, Decimal<Prec>> decimal_cast(T arg) {
  return Decimal<Prec>(arg);
}

template <int Prec, typename RoundPolicy, typename T>
constexpr impl::EnableIfInt<T, Decimal<Prec, RoundPolicy>> decimal_cast(T arg) {
  return Decimal<Prec, RoundPolicy>(arg);
}

template <int Prec, typename RoundPolicy = DefRoundPolicy>
constexpr Decimal<Prec, RoundPolicy> decimal_cast(double arg) {
  return Decimal<Prec, RoundPolicy>(arg);
}

template <int Prec, typename RoundPolicy = DefRoundPolicy>
constexpr Decimal<Prec, RoundPolicy> decimal_cast(long double arg) {
  return Decimal<Prec, RoundPolicy>(arg);
}

template <int Prec, typename RoundPolicy = DefRoundPolicy>
constexpr Decimal<Prec, RoundPolicy> decimal_cast(std::string_view arg) {
  return Decimal<Prec, RoundPolicy>(arg);
}

class ParseError : public std::runtime_error {
 public:
  ParseError(std::string_view source, size_t pos)
      : std::runtime_error(
            fmt::format(FMT_STRING("Error while parsing Decimal from string "
                                   "\"{}\", at position {}"),
                        source, pos + 1)) {}
};

namespace impl {

template <typename CharT>
bool IsSpace(CharT c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v';
}

template <typename CharT>
class StringCharSequence {
 public:
  explicit StringCharSequence(std::basic_string_view<CharT> sv)
      : current_(sv.data()), end_(sv.data() + sv.size()) {}

  // on sequence end, returns '\0'
  CharT Get() { return current_ == end_ ? CharT{'\0'} : *current_++; }

  void Unget() { --current_; }

  const char* Position() const { return current_; }

 private:
  const char* current_;
  const char* end_;
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
    const CharT c = in_->get();
    return c == kEof ? CharT{'\0'} : c;
  }

  void Unget() { in_->unget(); }

 private:
  std::basic_istream<CharT, Traits>* in_;
};

enum class ParseTolerance { kStrict, kAllowSpaces, kPermissive };

/// Extract values from a CharSequence ready to be packed to Decimal
template <typename CharSequence>
[[nodiscard]] bool ParseUnpacked(CharSequence input, int& sign, int64_t& before,
                                 int64_t& after, int& decimal_digits,
                                 ParseTolerance tolerance) {
  constexpr char dec_point = '.';

  enum class State {
    kSign,
    kBeforeFirstDig,
    kLeadingZeros,
    kBeforeDec,
    kAfterDec,
    kIgnoringAfterDec,
    kEnd
  };

  enum class Error { kOk, kWrongChar, kNoDigits, kOverflow, kWrongState };

  before = after = 0;
  sign = 1;

  auto state = State::kSign;
  auto error = Error::kOk;
  int before_digits_count = 0;
  int after_digit_count = 0;

  while (state != State::kEnd) {
    const auto c = input.Get();
    if (c == '\0') {
      break;
    }

    switch (state) {
      case State::kSign:
        if (c == '-') {
          sign = -1;
          state = State::kBeforeFirstDig;
        } else if (c == '+') {
          state = State::kBeforeFirstDig;
        } else if (c == '0') {
          state = State::kLeadingZeros;
          before_digits_count = 1;
        } else if ((c >= '1') && (c <= '9')) {
          state = State::kBeforeDec;
          before = static_cast<int>(c - '0');
          before_digits_count = 1;
        } else if (c == dec_point) {
          state = State::kAfterDec;
        } else if (IsSpace(c) && tolerance != ParseTolerance::kStrict) {
          // skip
        } else {
          state = State::kEnd;
          error = Error::kWrongChar;
        }
        break;
      case State::kBeforeFirstDig:
        if (c == '0') {
          state = State::kLeadingZeros;
          before_digits_count = 1;
        } else if ((c >= '1') && (c <= '9')) {
          state = State::kBeforeDec;
          before = static_cast<int>(c - '0');
          before_digits_count = 1;
        } else if (c == dec_point) {
          state = State::kAfterDec;
        } else {
          state = State::kEnd;
          error = Error::kWrongChar;
        }
        break;
      case State::kLeadingZeros:
        if (c == '0') {
          // skip
        } else if ((c >= '1') && (c <= '9')) {
          state = State::kBeforeDec;
          before = static_cast<int>(c - '0');
        } else if (c == dec_point) {
          state = State::kAfterDec;
        } else {
          state = State::kEnd;
        }
        break;
      case State::kBeforeDec:
        if ((c >= '0') && (c <= '9')) {
          if (before_digits_count < kMaxDecimalDigits) {
            before = 10 * before + static_cast<int>(c - '0');
            before_digits_count++;
          } else {
            error = Error::kOverflow;
          }
        } else if (c == dec_point) {
          state = State::kAfterDec;
        } else {
          state = State::kEnd;
        }
        break;
      case State::kAfterDec:
        if ((c >= '0') && (c <= '9')) {
          if (after_digit_count < kMaxDecimalDigits) {
            after = 10 * after + static_cast<int>(c - '0');
            after_digit_count++;
          } else {
            state = State::kIgnoringAfterDec;
            if (c >= '5') {
              // round half up
              after++;
            }
          }
        } else {
          state = State::kEnd;
        }
        break;
      case State::kIgnoringAfterDec:
        if ((c >= '0') && (c <= '9')) {
          // skip
        } else {
          state = State::kEnd;
        }
        break;
      default:
        error = Error::kWrongState;
        state = State::kEnd;
        break;
    }  // switch state
  }    // while has more chars & not end

  if (state == State::kEnd) {
    input.Unget();

    if (error == Error::kOk) {
      switch (tolerance) {
        case ParseTolerance::kStrict:
          error = Error::kWrongChar;
          break;
        case ParseTolerance::kAllowSpaces:
          while (true) {
            const auto c = input.Get();
            if (c == '\0') {
              break;
            } else if (!IsSpace(c)) {
              error = Error::kWrongChar;
              input.Unget();
              break;
            }
          }
          break;
        case ParseTolerance::kPermissive:
          break;
        default:
          error = Error::kWrongState;
          break;
      }
    }
  }

  if (error == Error::kOk && before_digits_count == 0 &&
      after_digit_count == 0) {
    error = Error::kNoDigits;
  }

  if (error == Error::kOk) {
    if (sign < 0) {
      before = -before;
      after = -after;
    }
    decimal_digits = after_digit_count;
  } else {
    sign = 0;
    before = 0;
    after = 0;
    decimal_digits = 0;
  }

  return error == Error::kOk;
}

/// Specifies how to treat the input containing more decimal digits than Prec
enum ParseRounding {
  /// Round according to the specified RoundPolicy
  kAllow,

  /// Treat rounding as an error, including trailing zeros beyond Prec
  kProhibit
};

/// Extract values from a CharSequence ready to be packed to Decimal
template <typename CharSequence, int Prec, typename RoundPolicy>
[[nodiscard]] bool Parse(CharSequence input, Decimal<Prec, RoundPolicy>& output,
                         ParseTolerance parse_tolerance,
                         ParseRounding parse_rounding) {
  int sign, after_digits;
  int64_t before, after;
  const bool success =
      ParseUnpacked(input, sign, before, after, after_digits, parse_tolerance);

  if (!success || before >= kMaxInt64 / kPow10<Prec>) {
    output = Decimal<Prec, RoundPolicy>(0);
    return false;
  }

  if (parse_rounding == ParseRounding::kProhibit && after_digits > Prec) {
    output = Decimal<Prec, RoundPolicy>(0);
    return false;
  }

  output =
      Decimal<Prec, RoundPolicy>::FromUnpacked(before, after, after_digits);
  return true;
}

// TODO TAXICOMMON-2894
// For Codegen only!
template <typename T>
T FromString(std::string_view input) {
  static_assert(IsDecimal<T>::value);

  impl::StringCharSequence source(input);
  T result;
  const bool success =
      impl::Parse(source, result, impl::ParseTolerance::kStrict,
                  impl::ParseRounding::kProhibit);

  if (!success) {
    throw ParseError(input, source.Position() - input.data());
  }
  return result;
}

enum class TrailingZerosMode { kLeave, kRemove };

// Returns the number of zeros trimmed
template <int Prec>
int TrimTrailingZeros(int64_t& after) {
  if constexpr (Prec == 0) {
    return 0;
  }
  if (after == 0) {
    return Prec;
  }

  int n_trimmed = 0;
  if constexpr (Prec >= 9) {
    if (after % 100000000 == 0) {
      after /= 100000000;
      n_trimmed += 8;
    }
  }
  if constexpr (Prec >= 5) {
    if (after % 10000 == 0) {
      after /= 10000;
      n_trimmed += 4;
    }
  }
  if constexpr (Prec >= 3) {
    if (after % 100 == 0) {
      after /= 100;
      n_trimmed += 2;
    }
  }
  if (after % 10 == 0) {
    after /= 10;
    n_trimmed += 1;
  }
  return n_trimmed;
}

template <int Prec, typename RoundPolicy>
std::string ToStringImpl(Decimal<Prec, RoundPolicy> dec,
                         TrailingZerosMode mode) {
  auto [before, after] = dec.AsUnpacked();
  int after_digits = Prec;

  if (mode == TrailingZerosMode::kRemove) {
    after_digits -= TrimTrailingZeros<Prec>(after);
  }

  if (after_digits > 0) {
    if (dec.Sign() == -1) {
      return fmt::format(FMT_STRING("-{}.{:0{}}"), -before, -after,
                         after_digits);
    } else {
      return fmt::format(FMT_STRING("{}.{:0{}}"), before, after, after_digits);
    }
  } else {
    return fmt::format(FMT_STRING("{}"), before);
  }
}

}  // namespace impl

template <int Prec, class RoundPolicy>
Decimal<Prec, RoundPolicy>::Decimal(std::string_view value) {
  // swallow the error
  static_cast<void>(impl::Parse(impl::StringCharSequence(value), *this,
                                impl::ParseTolerance::kPermissive,
                                impl::ParseRounding::kAllow));
}

/// Converts string to Decimal
/// Handles the following formats:
/// \code
/// 123
/// -123
/// 123.0
/// -123.0
/// 123.
/// .123
/// 0.
/// -.123
/// \endcode
/// Spaces and tabs on the front are ignored.
/// Performs rounding when provided value has higher precision than in output
/// type.
/// \param[input] input string
/// \param[output] output Decimal value, 0 on error
/// \result Returns true if conversion succeeded
template <int Prec, typename RoundPolicy>
bool fromString(std::string_view input, Decimal<Prec, RoundPolicy>& output) {
  return impl::Parse(impl::StringCharSequence(input), output,
                     impl::ParseTolerance::kAllowSpaces,
                     impl::ParseRounding::kAllow);
}

/// Converts string to Decimal
/// Handles the following formats:
/// \code
/// 123
/// -123
/// 123.0
/// -123.0
/// 123.
/// .123
/// 0.
/// -.123
/// \endcode
/// Spaces and tabs on the front are ignored.
/// Performs rounding when provided value has higher precision than in output
/// type.
/// \param[str] input string
/// \result Converted Decimal
/// \throws std::runtime_error if the string does not contain a Decimal
template <typename T>
T fromString(std::string_view str) {
  T output;
  // swallow the error
  static_cast<void>(fromString(str, output));
  return output;
}

/// Converts Decimal to a string. Trims any trailing zeros.
template <int Prec, typename RoundPolicy>
std::string ToString(Decimal<Prec, RoundPolicy> dec) {
  return impl::ToStringImpl(dec, impl::TrailingZerosMode::kRemove);
}

/// Converts Decimal to a string. Writes exactly Prec decimal digits, including
/// trailing zeros if needed.
template <int Prec, typename RoundPolicy>
std::string ToStringTrailingZeros(Decimal<Prec, RoundPolicy> dec) {
  return impl::ToStringImpl(dec, impl::TrailingZerosMode::kLeave);
}

// input
template <typename CharT, typename Traits, int Prec, typename RoundPolicy>
bool fromStream(std::basic_istream<CharT, Traits>& is,
                Decimal<Prec, RoundPolicy>& d) {
  const bool success = impl::Parse(impl::StreamCharSequence(is), d,
                                   impl::ParseTolerance::kPermissive,
                                   impl::ParseRounding::kAllow);
  if (!success) {
    is.setstate(std::ios_base::failbit);
  }
  return success;
}

// output
template <typename CharT, typename Traits, int Prec, typename RoundPolicy>
void toStream(std::basic_ostream<CharT, Traits>& os,
              Decimal<Prec, RoundPolicy> d) {
  os << ToString(d);
}

template <typename CharT, typename Traits, int Prec, typename RoundPolicy>
std::basic_istream<CharT, Traits>& operator>>(
    std::basic_istream<CharT, Traits>& is, Decimal<Prec, RoundPolicy>& d) {
  fromStream(is, d);
  return is;
}

template <typename CharT, typename Traits, int Prec, typename RoundPolicy>
std::basic_ostream<CharT, Traits>& operator<<(
    std::basic_ostream<CharT, Traits>& os,
    const Decimal<Prec, RoundPolicy>& d) {
  toStream(os, d);
  return os;
}

}  // namespace decimal64
