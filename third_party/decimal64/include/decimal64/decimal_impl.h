/////////////////////////////////////////////////////////////////////////////
// Name:        decimal.h
// Purpose:     Decimal data type support, for COBOL-like fixed-point
//              operations on currency values.
// Author:      Piotr Likus
// Created:     03/01/2011
// Last change: 23/09/2018
// Version:     1.16
// Licence:     BSD
/////////////////////////////////////////////////////////////////////////////

#ifndef _DECIMAL_H__
#define _DECIMAL_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file decimal.h
///
/// Decimal value type. Use for capital calculations.
/// Note: maximum handled value is: +9,223,372,036,854,775,807 (divided by prec)
///
/// Sample usage:
///   using namespace dec;
///   decimal<2> value(143125);
///   value = value / decimal_cast<2>(333);
///   cout << "Result is: " << value << endl;

// ----------------------------------------------------------------------------
// Config section
// ----------------------------------------------------------------------------
// - define DEC_EXTERNAL_INT64 if you do not want internal definition of "int64"
// data type
//   in this case define "DEC_INT64" somewhere
// - define DEC_EXTERNAL_ROUND if you do not want internal "round()" function
// - define DEC_CROSS_DOUBLE if you want to use double (instead of xdouble) for
// cross-conversions
// - define DEC_EXTERNAL_LIMITS to define by yourself DEC_MAX_INT32
// - define DEC_NO_CPP11 if your compiler does not support C++11
// - define DEC_TYPE_LEVEL as 0 for strong typing (same precision required for
// both arguments),
//   as 1 for allowing to mix lower or equal precision types
//   as 2 for automatic rounding when different precision is mixed

#include <fmt/format.h>

#include <ios>
#include <iosfwd>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

#ifndef DEC_TYPE_LEVEL
#define DEC_TYPE_LEVEL 2
#endif

// --> include headers for limits and int64_t

#ifndef DEC_NO_CPP11
#include <cstdint>
#include <limits>

#else

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#if defined(__GXX_EXPERIMENTAL_CXX0X) || (__cplusplus >= 201103L)
#include <cstdint>
#else
#include <stdint.h>
#endif  // defined
#endif  // DEC_NO_CPP11

// <--

// --> define DEC_MAX_INTxx, DEC_MIN_INTxx if required

#ifndef DEC_NAMESPACE
#define DEC_NAMESPACE dec
#endif  // DEC_NAMESPACE

#ifndef DEC_EXTERNAL_LIMITS
#ifndef DEC_NO_CPP11
//#define DEC_MAX_INT32 ((std::numeric_limits<int32_t>::max)())
#define DEC_MAX_INT64 ((std::numeric_limits<int64_t>::max)())
#define DEC_MIN_INT64 ((std::numeric_limits<int64_t>::min)())
#else
//#define DEC_MAX_INT32 INT32_MAX
#define DEC_MAX_INT64 INT64_MAX
#define DEC_MIN_INT64 INT64_MIN
#endif  // DEC_NO_CPP11
#endif  // DEC_EXTERNAL_LIMITS

// <--

namespace DEC_NAMESPACE {

// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------

// --> define DEC_INT64 if required
#ifndef DEC_EXTERNAL_INT64
#ifndef DEC_NO_CPP11
typedef int64_t DEC_INT64;
#else
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef signed __int64 DEC_INT64;
#else
typedef signed long long DEC_INT64;
#endif
#endif
#endif  // DEC_EXTERNAL_INT64
// <--

#ifdef DEC_NO_CPP11
#define static_assert(a, b)
#endif

typedef DEC_INT64 int64;
// type for storing currency value internally
typedef int64 dec_storage_t;
typedef unsigned int uint;
// xdouble is an "extended double" - can be long double, __float128, _Quad - as
// you wish
typedef long double xdouble;

#ifdef DEC_CROSS_DOUBLE
typedef double cross_float;
#else
typedef xdouble cross_float;
#endif

// ----------------------------------------------------------------------------
// Forward class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------
enum { max_decimal_points = 18 };

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------
template <int Prec>
struct DecimalFactor {
  static const int64 value = 10 * DecimalFactor<Prec - 1>::value;
};

template <>
struct DecimalFactor<0> {
  static const int64 value = 1;
};

template <>
struct DecimalFactor<1> {
  static const int64 value = 10;
};

template <int Prec, bool positive>
struct DecimalFactorDiff_impl {
  static const int64 value = DecimalFactor<Prec>::value;
};

template <int Prec>
struct DecimalFactorDiff_impl<Prec, false> {
  static const int64 value = INT64_MIN;
};

template <int Prec>
struct DecimalFactorDiff {
  static const int64 value = DecimalFactorDiff_impl<Prec, Prec >= 0>::value;
};

#ifndef DEC_EXTERNAL_ROUND

// round floating point value and convert to int64
template <class T>
inline int64 round(T value) {
  T val1;

  if (value < 0.0) {
    val1 = value - 0.5;
  } else {
    val1 = value + 0.5;
  }
  int64 intPart = static_cast<int64>(val1);

  return intPart;
}

// calculate output = round(a / b), where output, a, b are int64
inline bool div_rounded(int64& output, int64 a, int64 b) {
  int64 divisorCorr = std::abs(b) / 2;
  if (a >= 0) {
    if (DEC_MAX_INT64 - a >= divisorCorr) {
      output = (a + divisorCorr) / b;
      return true;
    }
  } else {
    if (-(DEC_MIN_INT64 - a) >= divisorCorr) {
      output = (a - divisorCorr) / b;
      return true;
    }
  }

  output = 0;
  return false;
}

#endif  // DEC_EXTERNAL_ROUND

template <class RoundPolicy>
class dec_utils {
 public:
  // result = (value1 * value2) / divisor
  inline static int64 multDiv(const int64 value1, const int64 value2,
                              int64 divisor) {
    // we don't check for division by zero, the caller should - the next line
    // will throw.
    const int64 value1int = value1 / divisor;
    int64 value1dec = value1 % divisor;
    const int64 value2int = value2 / divisor;
    int64 value2dec = value2 % divisor;

    int64 result = value1 * value2int + value1int * value2dec;

    if (value1dec == 0 || value2dec == 0) {
      return result;
    }

    if (!isMultOverflow(value1dec, value2dec)) {  // no overflow
      int64 resDecPart = value1dec * value2dec;
      if (!RoundPolicy::div_rounded(resDecPart, resDecPart, divisor))
        resDecPart = 0;
      result += resDecPart;
      return result;
    }

    // minimalize value1 & divisor
    {
      int64 c = gcd(value1dec, divisor);
      if (c != 1) {
        value1dec /= c;
        divisor /= c;
      }

      // minimalize value2 & divisor
      c = gcd(value2dec, divisor);
      if (c != 1) {
        value2dec /= c;
        divisor /= c;
      }
    }

    if (!isMultOverflow(value1dec, value2dec)) {  // no overflow
      int64 resDecPart = value1dec * value2dec;
      if (RoundPolicy::div_rounded(resDecPart, resDecPart, divisor)) {
        result += resDecPart;
        return result;
      }
    }

    // overflow can occur - use less precise version
    result += RoundPolicy::round(static_cast<cross_float>(value1dec) *
                                 static_cast<cross_float>(value2dec) /
                                 static_cast<cross_float>(divisor));
    return result;
  }

  static bool isMultOverflow(const int64 value1, const int64 value2) {
    if (value1 == 0 || value2 == 0) {
      return false;
    }

    if ((value1 < 0) != (value2 < 0)) {  // different sign
      if (value1 == DEC_MIN_INT64) {
        return value2 > 1;
      } else if (value2 == DEC_MIN_INT64) {
        return value1 > 1;
      }
      if (value1 < 0) {
        return isMultOverflow(-value1, value2);
      }
      if (value2 < 0) {
        return isMultOverflow(value1, -value2);
      }
    } else if (value1 < 0 && value2 < 0) {
      if (value1 == DEC_MIN_INT64) {
        return value2 < -1;
      } else if (value2 == DEC_MIN_INT64) {
        return value1 < -1;
      }
      return isMultOverflow(-value1, -value2);
    }

    return (value1 > DEC_MAX_INT64 / value2);
  }

  static int64 pow10(int n) {
    static constexpr int64 decimalFactorTable[] = {1,
                                                   10,
                                                   100,
                                                   1000,
                                                   10000,
                                                   100000,
                                                   1000000,
                                                   10000000,
                                                   100000000,
                                                   1000000000,
                                                   10000000000,
                                                   100000000000,
                                                   1000000000000,
                                                   10000000000000,
                                                   100000000000000,
                                                   1000000000000000,
                                                   10000000000000000,
                                                   100000000000000000,
                                                   1000000000000000000};

    if (n >= 0 && n <= max_decimal_points) {
      return decimalFactorTable[n];
    } else {
      return 0;
    }
  }

  template <class T>
  static int64 trunc(T value) {
    return static_cast<int64>(value);
  }

 private:
  // calculate greatest common divisor
  static int64 gcd(int64 a, int64 b) {
    int64 c;
    while (a != 0) {
      c = a;
      a = b % a;
      b = c;
    }
    return b;
  }
};

// no-rounding policy (decimal places stripped)
class null_round_policy {
 public:
  template <class T>
  static int64 round(T value) {
    return static_cast<int64>(value);
  }

  static bool div_rounded(int64& output, int64 a, int64 b) {
    output = a / b;
    return true;
  }
};

// default rounding policy - arithmetic, to nearest integer
class def_round_policy {
 public:
  template <class T>
  static int64 round(T value) {
    return DEC_NAMESPACE::round(value);
  }

  static bool div_rounded(int64& output, int64 a, int64 b) {
    return DEC_NAMESPACE::div_rounded(output, a, b);
  }
};

class half_down_round_policy {
 public:
  template <class T>
  static int64 round(T value) {
    T val1;
    T decimals;

    if (value >= 0.0) {
      decimals = value - floor(value);
      if (decimals > 0.5) {
        val1 = ceil(value);
      } else {
        val1 = value;
      }
    } else {
      decimals = std::abs(value + floor(std::abs(value)));
      if (decimals < 0.5) {
        val1 = ceil(value);
      } else {
        val1 = value;
      }
    }

    return static_cast<int64>(floor(val1));
  }

  static bool div_rounded(int64& output, int64 a, int64 b) {
    int64 divisorCorr = std::abs(b) / 2;
    int64 remainder = std::abs(a) % std::abs(b);

    if (a >= 0) {
      if (DEC_MAX_INT64 - a >= divisorCorr) {
        if (remainder > divisorCorr) {
          output = (a + divisorCorr) / b;
        } else {
          output = a / b;
        }
        return true;
      }
    } else {
      if (-(DEC_MIN_INT64 - a) >= divisorCorr) {
        output = (a - divisorCorr) / b;
        return true;
      }
    }

    output = 0;
    return false;
  }
};

class half_up_round_policy {
 public:
  template <class T>
  static int64 round(T value) {
    T val1;
    T decimals;

    if (value >= 0.0) {
      decimals = value - floor(value);
      if (decimals >= 0.5) {
        val1 = ceil(value);
      } else {
        val1 = value;
      }
    } else {
      decimals = std::abs(value + floor(std::abs(value)));
      if (decimals <= 0.5) {
        val1 = ceil(value);
      } else {
        val1 = value;
      }
    }

    return static_cast<int64>(floor(val1));
  }

  static bool div_rounded(int64& output, int64 a, int64 b) {
    int64 divisorCorr = std::abs(b) / 2;
    int64 remainder = std::abs(a) % std::abs(b);

    if (a >= 0) {
      if (DEC_MAX_INT64 - a >= divisorCorr) {
        if (remainder >= divisorCorr) {
          output = (a + divisorCorr) / b;
        } else {
          output = a / b;
        }
        return true;
      }
    } else {
      if (-(DEC_MIN_INT64 - a) >= divisorCorr) {
        if (remainder < divisorCorr) {
          output = (a - remainder) / b;
        } else if (remainder == divisorCorr) {
          output = (a + divisorCorr) / b;
        } else {
          output = (a + remainder - std::abs(b)) / b;
        }
        return true;
      }
    }

    output = 0;
    return false;
  }
};

// bankers' rounding
class half_even_round_policy {
 public:
  template <class T>
  static int64 round(T value) {
    T val1;
    T decimals;

    if (value >= 0.0) {
      decimals = value - floor(value);
      if (decimals > 0.5) {
        val1 = ceil(value);
      } else if (decimals < 0.5) {
        val1 = floor(value);
      } else {
        bool is_even = (static_cast<int64>(value - decimals) % 2 == 0);
        if (is_even) {
          val1 = floor(value);
        } else {
          val1 = ceil(value);
        }
      }
    } else {
      decimals = std::abs(value + floor(std::abs(value)));
      if (decimals > 0.5) {
        val1 = floor(value);
      } else if (decimals < 0.5) {
        val1 = ceil(value);
      } else {
        bool is_even = (static_cast<int64>(value + decimals) % 2 == 0);
        if (is_even) {
          val1 = ceil(value);
        } else {
          val1 = floor(value);
        }
      }
    }

    return static_cast<int64>(val1);
  }

  static bool div_rounded(int64& output, int64 a, int64 b) {
    int64 divisorDiv2 = std::abs(b) / 2;
    int64 remainder = std::abs(a) % std::abs(b);

    if (remainder == 0) {
      output = a / b;
    } else {
      if (a >= 0) {
        if (remainder > divisorDiv2) {
          output = (a - remainder + std::abs(b)) / b;
        } else if (remainder < divisorDiv2) {
          output = (a - remainder) / b;
        } else {
          bool is_even = std::abs(a / b) % 2 == 0;
          if (is_even) {
            output = a / b;
          } else {
            output = (a - remainder + std::abs(b)) / b;
          }
        }
      } else {
        // negative value
        if (remainder > divisorDiv2) {
          output = (a + remainder - std::abs(b)) / b;
        } else if (remainder < divisorDiv2) {
          output = (a + remainder) / b;
        } else {
          bool is_even = std::abs(a / b) % 2 == 0;
          if (is_even) {
            output = a / b;
          } else {
            output = (a + remainder - std::abs(b)) / b;
          }
        }
      }
    }

    return true;
  }
};

// round towards +infinity
class ceiling_round_policy {
 public:
  template <class T>
  static int64 round(T value) {
    return static_cast<int64>(ceil(value));
  }

  static bool div_rounded(int64& output, int64 a, int64 b) {
    int64 remainder = std::abs(a) % std::abs(b);
    if (remainder == 0) {
      output = a / b;
    } else {
      if (a >= 0) {
        output = (a + std::abs(b)) / b;
      } else {
        output = a / b;
      }
    }
    return true;
  }
};

// round towards -infinity
class floor_round_policy {
 public:
  template <class T>
  static int64 round(T value) {
    return static_cast<int64>(floor(value));
  }

  static bool div_rounded(int64& output, int64 a, int64 b) {
    int64 remainder = std::abs(a) % std::abs(b);
    if (remainder == 0) {
      output = a / b;
    } else {
      if (a >= 0) {
        output = (a - remainder) / b;
      } else {
        output = (a + remainder - std::abs(b)) / b;
      }
    }
    return true;
  }
};

// round towards zero = truncate
class round_down_round_policy : public null_round_policy {};

// round away from zero
class round_up_round_policy {
 public:
  template <class T>
  static int64 round(T value) {
    if (value >= 0.0) {
      return static_cast<int64>(ceil(value));
    } else {
      return static_cast<int64>(floor(value));
    }
  }

  static bool div_rounded(int64& output, int64 a, int64 b) {
    int64 remainder = std::abs(a) % std::abs(b);
    if (remainder == 0) {
      output = a / b;
    } else {
      if (a >= 0) {
        output = (a + std::abs(b)) / b;
      } else {
        output = (a - std::abs(b)) / b;
      }
    }
    return true;
  }
};

template <int Prec, class RoundPolicy = def_round_policy>
class decimal {
 public:
  typedef dec_storage_t raw_data_t;
  typedef RoundPolicy round_policy_t;
  enum { decimal_points = Prec };

  decimal() { init(0); }
  decimal(const decimal& src) { init(src); }
  explicit decimal(uint value) { init(value); }
  explicit decimal(int value) { init(value); }
  explicit decimal(int64 value) { init(value); }
  explicit decimal(xdouble value) { init(value); }
  explicit decimal(double value) { init(value); }
  explicit decimal(float value) { init(value); }
  explicit decimal(int64 value, int64 precFactor) {
    initWithPrec(value, precFactor);
  }
  explicit decimal(std::string_view value);

  ~decimal() {}

  static int64 getPrecFactor() { return DecimalFactor<Prec>::value; }
  static int getDecimalPoints() { return Prec; }

  decimal& operator=(const decimal& rhs) {
    if (&rhs != this) m_value = rhs.m_value;
    return *this;
  }

#if DEC_TYPE_LEVEL == 1
  template <int Prec2>
  typename std::enable_if<Prec >= Prec2, decimal>::type& operator=(
      const decimal<Prec2, RoundPolicy>& rhs) {
    m_value = rhs.getUnbiased() * DecimalFactorDiff<Prec - Prec2>::value;
    return *this;
  }
#elif DEC_TYPE_LEVEL > 1
  template <int Prec2>
  decimal& operator=(const decimal<Prec2, RoundPolicy>& rhs) {
    if (Prec2 > Prec) {
      RoundPolicy::div_rounded(m_value, rhs.getUnbiased(),
                               DecimalFactorDiff<Prec2 - Prec>::value);
    } else {
      m_value = rhs.getUnbiased() * DecimalFactorDiff<Prec - Prec2>::value;
    }
    return *this;
  }
#endif

  decimal& operator=(int64 rhs) {
    m_value = DecimalFactor<Prec>::value * rhs;
    return *this;
  }

  decimal& operator=(int rhs) {
    m_value = DecimalFactor<Prec>::value * rhs;
    return *this;
  }

  decimal& operator=(double rhs) {
    m_value = fpToStorage(rhs);
    return *this;
  }

  decimal& operator=(xdouble rhs) {
    m_value = fpToStorage(rhs);
    return *this;
  }

  bool operator==(const decimal& rhs) const { return (m_value == rhs.m_value); }

  bool operator<(const decimal& rhs) const { return (m_value < rhs.m_value); }

  bool operator<=(const decimal& rhs) const { return (m_value <= rhs.m_value); }

  bool operator>(const decimal& rhs) const { return (m_value > rhs.m_value); }

  bool operator>=(const decimal& rhs) const { return (m_value >= rhs.m_value); }

  bool operator!=(const decimal& rhs) const { return !(*this == rhs); }

  const decimal operator+(const decimal& rhs) const {
    decimal result = *this;
    result.m_value += rhs.m_value;
    return result;
  }

#if DEC_TYPE_LEVEL == 1
  template <int Prec2>
  const typename std::enable_if<Prec >= Prec2, decimal>::type operator+(
      const decimal<Prec2, RoundPolicy>& rhs) const {
    decimal result = *this;
    result.m_value +=
        rhs.getUnbiased() * DecimalFactorDiff<Prec - Prec2>::value;
    return result;
  }
#elif DEC_TYPE_LEVEL > 1
  template <int Prec2>
  const decimal operator+(const decimal<Prec2, RoundPolicy>& rhs) const {
    decimal result = *this;
    if (Prec2 > Prec) {
      int64 val;
      RoundPolicy::div_rounded(val, rhs.getUnbiased(),
                               DecimalFactorDiff<Prec2 - Prec>::value);
      result.m_value += val;
    } else {
      result.m_value +=
          rhs.getUnbiased() * DecimalFactorDiff<Prec - Prec2>::value;
    }

    return result;
  }
#endif

  decimal& operator+=(const decimal& rhs) {
    m_value += rhs.m_value;
    return *this;
  }

#if DEC_TYPE_LEVEL == 1
  template <int Prec2>
  typename std::enable_if<Prec >= Prec2, decimal>::type& operator+=(
      const decimal<Prec2, RoundPolicy>& rhs) {
    m_value += rhs.getUnbiased() * DecimalFactorDiff<Prec - Prec2>::value;
    return *this;
  }
#elif DEC_TYPE_LEVEL > 1
  template <int Prec2>
  decimal& operator+=(const decimal<Prec2, RoundPolicy>& rhs) {
    if (Prec2 > Prec) {
      int64 val;
      RoundPolicy::div_rounded(val, rhs.getUnbiased(),
                               DecimalFactorDiff<Prec2 - Prec>::value);
      m_value += val;
    } else {
      m_value += rhs.getUnbiased() * DecimalFactorDiff<Prec - Prec2>::value;
    }

    return *this;
  }
#endif

  const decimal operator+() const { return *this; }

  const decimal operator-() const {
    decimal result = *this;
    result.m_value = -result.m_value;
    return result;
  }

  const decimal operator-(const decimal& rhs) const {
    decimal result = *this;
    result.m_value -= rhs.m_value;
    return result;
  }

#if DEC_TYPE_LEVEL == 1
  template <int Prec2>
  const typename std::enable_if<Prec >= Prec2, decimal>::type operator-(
      const decimal<Prec2, RoundPolicy>& rhs) const {
    decimal result = *this;
    result.m_value -=
        rhs.getUnbiased() * DecimalFactorDiff<Prec - Prec2>::value;
    return result;
  }
#elif DEC_TYPE_LEVEL > 1
  template <int Prec2>
  const decimal operator-(const decimal<Prec2, RoundPolicy>& rhs) const {
    decimal result = *this;
    if (Prec2 > Prec) {
      int64 val;
      RoundPolicy::div_rounded(val, rhs.getUnbiased(),
                               DecimalFactorDiff<Prec2 - Prec>::value);
      result.m_value -= val;
    } else {
      result.m_value -=
          rhs.getUnbiased() * DecimalFactorDiff<Prec - Prec2>::value;
    }

    return result;
  }
#endif

  decimal& operator-=(const decimal& rhs) {
    m_value -= rhs.m_value;
    return *this;
  }

#if DEC_TYPE_LEVEL == 1
  template <int Prec2>
  typename std::enable_if<Prec >= Prec2, decimal>::type& operator-=(
      const decimal<Prec2, RoundPolicy>& rhs) {
    m_value -= rhs.getUnbiased() * DecimalFactorDiff<Prec - Prec2>::value;
    return *this;
  }
#elif DEC_TYPE_LEVEL > 1
  template <int Prec2>
  decimal& operator-=(const decimal<Prec2, RoundPolicy>& rhs) {
    if (Prec2 > Prec) {
      int64 val;
      RoundPolicy::div_rounded(val, rhs.getUnbiased(),
                               DecimalFactorDiff<Prec2 - Prec>::value);
      m_value -= val;
    } else {
      m_value -= rhs.getUnbiased() * DecimalFactorDiff<Prec - Prec2>::value;
    }

    return *this;
  }
#endif

  const decimal operator*(int rhs) const {
    decimal result = *this;
    result.m_value *= rhs;
    return result;
  }

  const decimal operator*(int64 rhs) const {
    decimal result = *this;
    result.m_value *= rhs;
    return result;
  }

  const decimal operator*(const decimal& rhs) const {
    decimal result = *this;
    result.m_value = dec_utils<RoundPolicy>::multDiv(
        result.m_value, rhs.m_value, DecimalFactor<Prec>::value);
    return result;
  }

#if DEC_TYPE_LEVEL == 1
  template <int Prec2>
  const typename std::enable_if<Prec >= Prec2, decimal>::type operator*(
      const decimal<Prec2, RoundPolicy>& rhs) const {
    decimal result = *this;
    result.m_value = dec_utils<RoundPolicy>::multDiv(
        result.m_value, rhs.getUnbiased(), DecimalFactor<Prec2>::value);
    return result;
  }
#elif DEC_TYPE_LEVEL > 1
  template <int Prec2>
  const decimal operator*(const decimal<Prec2, RoundPolicy>& rhs) const {
    decimal result = *this;
    result.m_value = dec_utils<RoundPolicy>::multDiv(
        result.m_value, rhs.getUnbiased(), DecimalFactor<Prec2>::value);
    return result;
  }
#endif

  decimal& operator*=(int rhs) {
    m_value *= rhs;
    return *this;
  }

  decimal& operator*=(int64 rhs) {
    m_value *= rhs;
    return *this;
  }

  decimal& operator*=(const decimal& rhs) {
    m_value = dec_utils<RoundPolicy>::multDiv(m_value, rhs.m_value,
                                              DecimalFactor<Prec>::value);
    return *this;
  }

#if DEC_TYPE_LEVEL == 1
  template <int Prec2>
  typename std::enable_if<Prec >= Prec2, decimal>::type& operator*=(
      const decimal<Prec2, RoundPolicy>& rhs) {
    m_value = dec_utils<RoundPolicy>::multDiv(m_value, rhs.getUnbiased(),
                                              DecimalFactor<Prec2>::value);
    return *this;
  }
#elif DEC_TYPE_LEVEL > 1
  template <int Prec2>
  decimal& operator*=(const decimal<Prec2, RoundPolicy>& rhs) {
    m_value = dec_utils<RoundPolicy>::multDiv(m_value, rhs.getUnbiased(),
                                              DecimalFactor<Prec2>::value);
    return *this;
  }
#endif

  const decimal operator/(int rhs) const {
    decimal result = *this;

    if (!RoundPolicy::div_rounded(result.m_value, this->m_value, rhs)) {
      result.m_value = dec_utils<RoundPolicy>::multDiv(result.m_value, 1, rhs);
    }

    return result;
  }

  const decimal operator/(int64 rhs) const {
    decimal result = *this;

    if (!RoundPolicy::div_rounded(result.m_value, this->m_value, rhs)) {
      result.m_value = dec_utils<RoundPolicy>::multDiv(result.m_value, 1, rhs);
    }

    return result;
  }

  const decimal operator/(const decimal& rhs) const {
    decimal result = *this;
    // result.m_value = (result.m_value * DecimalFactor<Prec>::value) /
    // rhs.m_value;
    result.m_value = dec_utils<RoundPolicy>::multDiv(
        result.m_value, DecimalFactor<Prec>::value, rhs.m_value);

    return result;
  }

#if DEC_TYPE_LEVEL == 1
  template <int Prec2>
  const typename std::enable_if<Prec >= Prec2, decimal>::type operator/(
      const decimal<Prec2, RoundPolicy>& rhs) const {
    decimal result = *this;
    result.m_value = dec_utils<RoundPolicy>::multDiv(
        result.m_value, DecimalFactor<Prec2>::value, rhs.getUnbiased());
    return result;
  }
#elif DEC_TYPE_LEVEL > 1
  template <int Prec2>
  const decimal operator/(const decimal<Prec2, RoundPolicy>& rhs) const {
    decimal result = *this;
    result.m_value = dec_utils<RoundPolicy>::multDiv(
        result.m_value, DecimalFactor<Prec2>::value, rhs.getUnbiased());
    return result;
  }
#endif

  decimal& operator/=(int rhs) {
    if (!RoundPolicy::div_rounded(this->m_value, this->m_value, rhs)) {
      this->m_value = dec_utils<RoundPolicy>::multDiv(this->m_value, 1, rhs);
    }
    return *this;
  }

  decimal& operator/=(int64 rhs) {
    if (!RoundPolicy::div_rounded(this->m_value, this->m_value, rhs)) {
      this->m_value = dec_utils<RoundPolicy>::multDiv(this->m_value, 1, rhs);
    }
    return *this;
  }

  decimal& operator/=(const decimal& rhs) {
    // m_value = (m_value * DecimalFactor<Prec>::value) / rhs.m_value;
    m_value = dec_utils<RoundPolicy>::multDiv(
        m_value, DecimalFactor<Prec>::value, rhs.m_value);

    return *this;
  }

  /// Returns integer indicating sign of value
  /// -1 if value is < 0
  /// +1 if value is > 0
  /// 0  if value is 0
  int sign() const { return (m_value > 0) ? 1 : ((m_value < 0) ? -1 : 0); }

#if DEC_TYPE_LEVEL == 1
  template <int Prec2>
  typename std::enable_if<Prec >= Prec2, decimal>::type& operator/=(
      const decimal<Prec2, RoundPolicy>& rhs) {
    m_value = dec_utils<RoundPolicy>::multDiv(
        m_value, DecimalFactor<Prec2>::value, rhs.getUnbiased());

    return *this;
  }
#elif DEC_TYPE_LEVEL > 1
  template <int Prec2>
  decimal& operator/=(const decimal<Prec2, RoundPolicy>& rhs) {
    m_value = dec_utils<RoundPolicy>::multDiv(
        m_value, DecimalFactor<Prec2>::value, rhs.getUnbiased());

    return *this;
  }
#endif

  double getAsDouble() const {
    return static_cast<double>(m_value) / getPrecFactorDouble();
  }

  void setAsDouble(double value) { m_value = fpToStorage(value); }

  xdouble getAsXDouble() const {
    return static_cast<xdouble>(m_value) / getPrecFactorXDouble();
  }

  void setAsXDouble(xdouble value) { m_value = fpToStorage(value); }

  // returns integer value = real_value * (10 ^ precision)
  // use to load/store decimal value in external memory
  int64 getUnbiased() const { return m_value; }
  void setUnbiased(int64 value) { m_value = value; }

  decimal<Prec, RoundPolicy> abs() const {
    if (m_value >= 0)
      return *this;
    else
      return (decimal<Prec, RoundPolicy>(0) - *this);
  }

  /// returns value rounded to integer using active rounding policy
  int64 getAsInteger() const {
    int64 result;
    RoundPolicy::div_rounded(result, m_value, DecimalFactor<Prec>::value);
    return result;
  }

  /// overwrites internal value with integer
  void setAsInteger(int64 value) {
    m_value = DecimalFactor<Prec>::value * value;
  }

  /// Returns two parts: before and after decimal point
  /// For negative values both numbers are negative or zero.
  void unpack(int64& beforeValue, int64& afterValue) const {
    afterValue = m_value % DecimalFactor<Prec>::value;
    beforeValue = (m_value - afterValue) / DecimalFactor<Prec>::value;
  }

  /// Combines two parts (before and after decimal point) into decimal value.
  /// Both input values have to have the same sign for correct results.
  /// Does not perform any rounding or input validation - afterValue must be
  /// less than 10^prec.
  /// \param[in] beforeValue value before decimal point
  /// \param[in] afterValue value after decimal point multiplied by 10^prec
  /// \result Returns *this
  decimal& pack(int64 beforeValue, int64 afterValue) {
    if (Prec > 0) {
      m_value = beforeValue * DecimalFactor<Prec>::value;
      m_value += (afterValue % DecimalFactor<Prec>::value);
    } else
      m_value = beforeValue * DecimalFactor<Prec>::value;
    return *this;
  }

  /// Version of pack() with rounding, sourcePrec specifies precision of source
  /// values. See also @pack.
  template <int sourcePrec>
  decimal& pack_rounded(int64 beforeValue, int64 afterValue) {
    decimal<sourcePrec, RoundPolicy> temp;
    temp.pack(beforeValue, afterValue);
    decimal<Prec, RoundPolicy> result(temp.getUnbiased(), temp.getPrecFactor());

    *this = result;
    return *this;
  }

  static decimal buildWithExponent(int64 mantissa, int exponent) {
    decimal result;
    result.setWithExponent(mantissa, exponent);
    return result;
  }

  static decimal& buildWithExponent(decimal& output, int64 mantissa,
                                    int exponent) {
    output.setWithExponent(mantissa, exponent);
    return output;
  }

  void setWithExponent(int64 mantissa, int exponent) {
    int exponentForPack = exponent + Prec;

    if (exponentForPack < 0) {
      int64 newValue;

      if (!RoundPolicy::div_rounded(
              newValue, mantissa,
              dec_utils<RoundPolicy>::pow10(-exponentForPack))) {
        newValue = 0;
      }

      m_value = newValue;
    } else {
      m_value = mantissa * dec_utils<RoundPolicy>::pow10(exponentForPack);
    }
  }

  void getWithExponent(int64& mantissa, int& exponent) const {
    int64 value = m_value;
    int exp = -Prec;

    if (value != 0) {
      // normalize
      while (value % 10 == 0) {
        value /= 10;
        exp++;
      }
    }

    mantissa = value;
    exponent = exp;
  }

 protected:
  inline xdouble getPrecFactorXDouble() const {
    return static_cast<xdouble>(DecimalFactor<Prec>::value);
  }

  inline double getPrecFactorDouble() const {
    return static_cast<double>(DecimalFactor<Prec>::value);
  }

  void init(const decimal& src) { m_value = src.m_value; }

  void init(uint value) { m_value = DecimalFactor<Prec>::value * value; }

  void init(int value) { m_value = DecimalFactor<Prec>::value * value; }

  void init(int64 value) { m_value = DecimalFactor<Prec>::value * value; }

  void init(xdouble value) { m_value = fpToStorage(value); }

  void init(double value) { m_value = fpToStorage(value); }

  void init(float value) { m_value = fpToStorage(static_cast<double>(value)); }

  void initWithPrec(int64 value, int64 precFactor) {
    int64 ownFactor = DecimalFactor<Prec>::value;

    if (ownFactor == precFactor) {
      // no conversion required
      m_value = value;
    } else {
      // conversion
      m_value = RoundPolicy::round(static_cast<cross_float>(value) *
                                   (static_cast<cross_float>(ownFactor) /
                                    static_cast<cross_float>(precFactor)));
    }
  }

  template <typename T>
  static dec_storage_t fpToStorage(T value) {
    dec_storage_t intPart = dec_utils<RoundPolicy>::trunc(value);
    T fracPart = value - intPart;
    return RoundPolicy::round(static_cast<T>(DecimalFactor<Prec>::value) *
                              fracPart) +
           DecimalFactor<Prec>::value * intPart;
  }

  template <typename T>
  static T abs(T value) {
    if (value < 0)
      return -value;
    else
      return value;
  }

 protected:
  dec_storage_t m_value;
};

// ----------------------------------------------------------------------------
// Pre-defined types
// ----------------------------------------------------------------------------
typedef decimal<2> decimal2;
typedef decimal<4> decimal4;
typedef decimal<6> decimal6;

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------
template <int Prec, class T, class RoundPolicy = typename T::round_policy_t>
decimal<Prec, RoundPolicy> decimal_cast(const T& arg) {
  return decimal<Prec, RoundPolicy>(arg.getUnbiased(), arg.getPrecFactor());
}

// Example of use:
//   c = dec::decimal_cast<6>(a * b);
template <int Prec>
decimal<Prec> decimal_cast(uint arg) {
  decimal<Prec> result(arg);
  return result;
}

template <int Prec>
decimal<Prec> decimal_cast(int arg) {
  decimal<Prec> result(arg);
  return result;
}

template <int Prec>
decimal<Prec> decimal_cast(int64 arg) {
  decimal<Prec> result(arg);
  return result;
}

template <int Prec>
decimal<Prec> decimal_cast(double arg) {
  decimal<Prec> result(arg);
  return result;
}

template <int Prec>
decimal<Prec> decimal_cast(std::string_view arg) {
  decimal<Prec> result(arg);
  return result;
}

template <int Prec, int N>
decimal<Prec> decimal_cast(const char (&arg)[N]) {
  decimal<Prec> result(arg);
  return result;
}

// with rounding policy
template <int Prec, typename RoundPolicy>
decimal<Prec, RoundPolicy> decimal_cast(uint arg) {
  decimal<Prec, RoundPolicy> result(arg);
  return result;
}

template <int Prec, typename RoundPolicy>
decimal<Prec, RoundPolicy> decimal_cast(int arg) {
  decimal<Prec, RoundPolicy> result(arg);
  return result;
}

template <int Prec, typename RoundPolicy>
decimal<Prec, RoundPolicy> decimal_cast(int64 arg) {
  decimal<Prec, RoundPolicy> result(arg);
  return result;
}

template <int Prec, typename RoundPolicy>
decimal<Prec, RoundPolicy> decimal_cast(double arg) {
  decimal<Prec, RoundPolicy> result(arg);
  return result;
}

template <int Prec, typename RoundPolicy>
decimal<Prec, RoundPolicy> decimal_cast(std::string_view arg) {
  decimal<Prec, RoundPolicy> result(arg);
  return result;
}

template <int Prec, typename RoundPolicy, int N>
decimal<Prec, RoundPolicy> decimal_cast(const char (&arg)[N]) {
  decimal<Prec, RoundPolicy> result(arg);
  return result;
}

namespace details {

template <typename CharT>
char narrow(CharT c) {
  // assume there are no decimal-related characters outside of `char` domain
  return c >= std::numeric_limits<char>::min() &&
                 c <= std::numeric_limits<char>::max()
             ? static_cast<char>(c)
             : 'a';
}

template <typename CharT>
class StringCharSequence {
 public:
  explicit StringCharSequence(std::basic_string_view<CharT> sv)
      : current_(sv.cbegin()), end_(sv.cend()) {}

  /// on sequence end, returns '\0'
  char get() { return current_ == end_ ? '\0' : narrow(*current_++); }

  void unget() { --current_; }

 private:
  typename std::basic_string_view<CharT>::const_iterator current_;
  typename std::basic_string_view<CharT>::const_iterator end_;
};

template <typename CharT, typename Traits>
class StreamCharSequence {
 public:
  explicit StreamCharSequence(std::basic_istream<CharT, Traits>& in)
      : in_(&in) {}

  /// on sequence end, returns '\0'
  char get() {
    constexpr auto eof = std::basic_istream<CharT, Traits>::traits_type::eof();
    if (!in_->good()) {
      return '\0';
    }
    const auto c = in_->get();
    return c == eof ? '\0' : narrow(c);
  }

  void unget() { in_->unget(); }

 private:
  std::basic_istream<CharT, Traits>* in_;
};

enum class ParsingMode { strict, allowSpaces, permissive };

/// Extract values from a CharSequence ready to be packed to decimal
template <typename CharSequence>
[[nodiscard]] bool parseUnpacked(CharSequence input, int& sign, int64& before,
                                 int64& after, int& decimalDigits,
                                 ParsingMode mode) {
  constexpr char decPoint = '.';

  enum StateEnum {
    IN_SIGN,
    IN_BEFORE_FIRST_DIG,
    IN_LEADING_ZEROS,
    IN_BEFORE_DEC,
    IN_AFTER_DEC,
    IN_IGNORING_AFTER_DEC,
    IN_END
  };

  enum ErrorCodes {
    ERR_WRONG_CHAR = -1,
    ERR_NO_DIGITS = -2,
    ERR_OVERFLOW = -3,
    ERR_WRONG_STATE = -4
  };

  const auto isSpace = [](char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v';
  };

  before = after = 0;
  sign = 1;

  StateEnum state = IN_SIGN;
  int error = 0;
  int beforeDigitsCount = 0;
  int afterDigitCount = 0;

  while (state != IN_END) {
    const char c = input.get();
    if (c == '\0') {
      break;
    }

    switch (state) {
      case IN_SIGN:
        if (c == '-') {
          sign = -1;
          state = IN_BEFORE_FIRST_DIG;
        } else if (c == '+') {
          state = IN_BEFORE_FIRST_DIG;
        } else if (c == '0') {
          state = IN_LEADING_ZEROS;
          beforeDigitsCount = 1;
        } else if ((c >= '1') && (c <= '9')) {
          state = IN_BEFORE_DEC;
          before = static_cast<int>(c - '0');
          beforeDigitsCount = 1;
        } else if (c == decPoint) {
          state = IN_AFTER_DEC;
        } else if (isSpace(c) && mode != ParsingMode::strict) {
          // skip
        } else {
          state = IN_END;
          error = ERR_WRONG_CHAR;
        }
        break;
      case IN_BEFORE_FIRST_DIG:
        if (c == '0') {
          state = IN_LEADING_ZEROS;
          beforeDigitsCount = 1;
        } else if ((c >= '1') && (c <= '9')) {
          state = IN_BEFORE_DEC;
          before = static_cast<int>(c - '0');
          beforeDigitsCount = 1;
        } else if (c == decPoint) {
          state = IN_AFTER_DEC;
        } else {
          state = IN_END;
          error = ERR_WRONG_CHAR;
        }
        break;
      case IN_LEADING_ZEROS:
        if (c == '0') {
          // skip
        } else if ((c >= '1') && (c <= '9')) {
          state = IN_BEFORE_DEC;
          before = static_cast<int>(c - '0');
        } else if (c == decPoint) {
          state = IN_AFTER_DEC;
        } else {
          state = IN_END;
        }
        break;
      case IN_BEFORE_DEC:
        if ((c >= '0') && (c <= '9')) {
          if (beforeDigitsCount < max_decimal_points) {
            before = 10 * before + static_cast<int>(c - '0');
            beforeDigitsCount++;
          } else {
            error = ERR_OVERFLOW;
          }
        } else if (c == decPoint) {
          state = IN_AFTER_DEC;
        } else {
          state = IN_END;
        }
        break;
      case IN_AFTER_DEC:
        if ((c >= '0') && (c <= '9')) {
          if (afterDigitCount < max_decimal_points) {
            after = 10 * after + static_cast<int>(c - '0');
            afterDigitCount++;
          } else {
            state = IN_IGNORING_AFTER_DEC;
            if (c >= '5') {
              // round half up
              after++;
            }
          }
        } else {
          state = IN_END;
        }
        break;
      case IN_IGNORING_AFTER_DEC:
        if ((c >= '0') && (c <= '9')) {
          // skip
        } else {
          state = IN_END;
        }
        break;
      default:
        error = ERR_WRONG_STATE;
        state = IN_END;
        break;
    }  // switch state
  }  // while has more chars & not end

  if (state == IN_END) {
    input.unget();

    if (error >= 0) {
      switch (mode) {
        case ParsingMode::strict:
          error = ERR_WRONG_CHAR;
          break;
        case ParsingMode::allowSpaces:
          while (true) {
            const char c = input.get();
            if (c == '\0') {
              break;
            } else if (!isSpace(c)) {
              error = ERR_WRONG_CHAR;
              input.unget();
              break;
            }
          }
          break;
        case ParsingMode::permissive:
          break;
        default:
          error = ERR_WRONG_STATE;
          break;
      }
    }
  }

  if (error >= 0 && beforeDigitsCount == 0 && afterDigitCount == 0) {
    error = ERR_NO_DIGITS;
  }

  if (error < 0) {
    sign = 0;
    before = 0;
    after = 0;
    decimalDigits = 0;
  } else {
    if (sign < 0) {
      before = -before;
      after = -after;
    }
    decimalDigits = afterDigitCount;
  }

  return error >= 0;
}  // function

/// Extract values from a CharSequence ready to be packed to decimal
template <typename CharSequence, int Prec, typename RoundPolicy>
[[nodiscard]] bool parse(CharSequence input, decimal<Prec, RoundPolicy>& output,
           ParsingMode mode) {
  int sign, afterDigits;
  int64 before, after;
  const bool success =
      details::parseUnpacked(input, sign, before, after, afterDigits, mode);

  if (!success || before >= DecimalFactor<max_decimal_points - Prec>::value) {
    output = decimal<Prec, RoundPolicy>(0);
    return false;
  }

  if (afterDigits <= Prec) {
    // direct mode
    const int missingDigits = Prec - afterDigits;
    const int64 factor = dec_utils<RoundPolicy>::pow10(missingDigits);
    output.pack(before, after * factor);
  } else {
    // rounding mode
    const int extraDigits = afterDigits - Prec;
    const int64 factor = dec_utils<RoundPolicy>::pow10(extraDigits);
    int64 roundedAfter;
    RoundPolicy::div_rounded(roundedAfter, after, factor);
    output.pack(before, roundedAfter);
  }
  return true;
}  // function

inline void checkParsingSuccess(bool success) {
  if (!success) {
    throw std::runtime_error("Could not convert the string to decimal");
  }
}

}  // namespace details

template <int Prec, class RoundPolicy>
decimal<Prec, RoundPolicy>::decimal(std::string_view value) {
  // swallow the error
  static_cast<void>(details::parse(details::StringCharSequence(value), *this,
                     details::ParsingMode::permissive));
}

/// Converts string to decimal
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
/// \param[output] output decimal value, 0 on error
/// \result Returns true if conversion succeeded
template <int Prec, typename RoundPolicy>
bool fromString(std::string_view input,
                              decimal<Prec, RoundPolicy>& output) {
  return details::parse(details::StringCharSequence(input), output,
                        details::ParsingMode::allowSpaces);
}

/// Converts string to decimal
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
/// \result Converted decimal
/// \throws std::runtime_error if the string does not contain a decimal
template <typename T>
T fromString(std::string_view str) {
  T output;
  // swallow the error
  static_cast<void>(fromString(str, output));
  return output;
}

/// Exports decimal to string
/// Used format: {-}bbb.aaa where
/// - {-} is optional '-' sign character
/// - bbb is stream of digits before decimal point
/// - aaa is stream of digits after decimal point
template <int Prec, typename RoundPolicy>
std::string toString(decimal<Prec, RoundPolicy> arg) {
  int64 before, after;
  arg.unpack(before, after);

  if (Prec > 0) {
    if (before < 0 || after < 0) {
      return fmt::format(FMT_STRING("-{}.{:0{}}"), -before, -after, Prec);
    } else {
      return fmt::format(FMT_STRING("{}.{:0{}}"), before, after, Prec);
    }
  } else {
    return fmt::format(FMT_STRING("{}"), before);
  }
}

// input
template <typename CharT, typename Traits, int Prec, typename RoundPolicy>
bool fromStream(std::basic_istream<CharT, Traits>& is,
                              decimal<Prec, RoundPolicy>& d) {
  const bool success = details::parse(details::StreamCharSequence(is), d,
                                      details::ParsingMode::permissive);
  if (!success) {
    is.setstate(std::ios_base::failbit);
  }
  return success;
}

// output
template <typename CharT, typename Traits, int Prec, typename RoundPolicy>
void toStream(std::basic_ostream<CharT, Traits>& os,
              decimal<Prec, RoundPolicy> d) {
  os << toString(d);
}

template <typename CharT, typename Traits, int Prec, typename RoundPolicy>
std::basic_istream<CharT, Traits>& operator>>(
    std::basic_istream<CharT, Traits>& is, decimal<Prec, RoundPolicy>& d) {
  fromStream(is, d);
  return is;
}

template <typename CharT, typename Traits, int Prec, typename RoundPolicy>
std::basic_ostream<CharT, Traits>& operator<<(
    std::basic_ostream<CharT, Traits>& os,
    const decimal<Prec, RoundPolicy>& d) {
  toStream(os, d);
  return os;
}

}  // namespace DEC_NAMESPACE
#endif  // _DECIMAL_H__
