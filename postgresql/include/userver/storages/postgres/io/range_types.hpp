#pragma once

/// @file userver/storages/postgres/io/range_types.hpp
/// @brief Ranges I/O support
/// @ingroup userver_postgres_parse_and_format

/// @page pg_range_types uPg: Range types
///
/// PostgreSQL provides a facility to represent intervals of values. They can be
/// bounded (have both ends), e.g [0, 1], [2, 10), unbounded (at least one end
/// is infinity or absent), e.g. (-∞, +∞), and empty.
///
/// The range that can be unbounded is modelled by storages::postgres::Range<>
/// template, which provides means of constructing any possible combination of
/// interval ends. It is very versatile, but not very convenient in most cases.
/// storages::postgres::BoundedRange<> template is an utility that works only
/// with bounded ranges.
///
/// @par PostgreSQL Built-in Range Datatypes
///
/// Some of PostgreSQL built-in range datatypes
/// (https://www.postgresql.org/docs/current/rangetypes.html#RANGETYPES-BUILTIN)
/// are supported by the library, please see @ref pg_types
///
/// @par User Range Types
///
/// A user can define custom range types
/// (https://www.postgresql.org/docs/current/rangetypes.html#RANGETYPES-DEFINING)
/// and they can be mapped to C++ counterparts, e.g.
///
/// @code
/// CREATE TYPE my_range AS RANGE (
///   subtype = my_type
/// );
/// @endcode
///
/// and declare a mapping from C++ range to this type just as for any other user
/// type:
///
/// @code
/// template<>
/// struct CppToUserPg<Range<my_type>> {
///   static constexpr DBTypeName postgres_name = "my_type";
/// };
/// @endcode
///
/// Please note that `my_type` must be comparable both in Postgres and in C++
///
/// See @ref pg_user_types for more info
///
/// @par Time Range and Other Widely Used Types
///
/// If you need a range of PostgreSQL `float` type or `time` type (actually any
/// type mapped to C++ type that is highly likely used by other developers),
/// please DO NOT specialize mapping for `Range<float>`, `Range<double>` or
/// `Range<TimeOfDay<seconds>>`. Please declare a strong typedef for your range
/// or bounded range and map it to your postgres range type.
///
/// Here is an example how to define a strong typedef for a range of TimeOfDay
/// and map it to postgres user range type.
///
/// @snippet storages/postgres/tests/user_types_pgtest.cpp Time range
///
/// @snippet storages/postgres/tests/user_types_pgtest.cpp Range type mapping
///
///
/// ----------
///
/// @htmlonly <div class="bottom-nav"> @endhtmlonly
/// ⇦ @ref pg_enum | @ref pg_arrays ⇨
/// @htmlonly </div> @endhtmlonly

#include <optional>
#include <ostream>

#include <fmt/format.h>

#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/buffer_io_base.hpp>
#include <userver/storages/postgres/io/field_buffer.hpp>
#include <userver/storages/postgres/io/traits.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>
#include <userver/storages/postgres/io/type_traits.hpp>
#include <userver/storages/postgres/io/user_types.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/flags.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

struct UnboundedType {};
constexpr UnboundedType kUnbounded{};

enum class RangeBound {
  kNone = 0x00,
  kLower = 0x01,
  kUpper = 0x02,
  kBoth = kLower | kUpper,
};

using RangeBounds = USERVER_NAMESPACE::utils::Flags<RangeBound>;

template <typename T>
class Range {
  static constexpr bool kNothrowValueCtor =
      std::is_nothrow_default_constructible_v<T>;
  static constexpr bool kNothrowValueCopy =
      std::is_nothrow_copy_constructible_v<T>;
  static constexpr bool kNothrowValueMove =
      std::is_nothrow_move_constructible_v<T>;
  static constexpr bool kIsDiscreteValue = std::is_integral_v<T>;

 public:
  using OptionalValue = std::optional<T>;

  /// Empty range
  Range() = default;

  /// Unbounded range
  Range(UnboundedType, UnboundedType) noexcept : data{RangeData{}} {}

  /// Bounded range
  template <typename U, typename = std::enable_if_t<
                            std::is_convertible_v<std::decay_t<U>, T>>>
  Range(U&& lower, U&& upper, RangeBounds bounds = RangeBound::kLower);

  /// Range with a lower bound
  template <typename U, typename = std::enable_if_t<
                            std::is_convertible_v<std::decay_t<U>, T>>>
  Range(U&& lower, UnboundedType ub,
        RangeBounds bounds = RangeBound::kLower) noexcept(kNothrowValueCopy);

  /// Range with an upper bound
  template <typename U, typename = std::enable_if_t<
                            std::is_convertible_v<std::decay_t<U>, T>>>
  Range(UnboundedType ub, U&& upper,
        RangeBounds bounds = RangeBound::kNone) noexcept(kNothrowValueCopy);

  Range(const OptionalValue& lower, const OptionalValue& upper,
        RangeBounds bounds);

  /// Convert from a range of different type.
  ///
  /// Intentionally implicit
  template <typename U,
            typename = std::enable_if_t<std::is_convertible_v<U, T>>>
  Range(const Range<U>& rhs);

  bool operator==(const Range& rhs) const;

  bool operator!=(const Range& rhs) const { return !(*this == rhs); }

  bool Empty() const { return !data; }

  /// Make the range empty
  void Clear() { data.reset(); }

  bool HasLowerBound() const {
    return !!data && data->HasBound(RangeBound::kLower);
  }
  bool HasUpperBound() const {
    return !!data && data->HasBound(RangeBound::kUpper);
  }

  /// Get the lower bound.
  const OptionalValue& GetLowerBound() const {
    if (!!data) {
      return data->GetOptionalValue(RangeBound::kLower);
    }
    return kNoValue;
  }

  /// Get the upper bound.
  const OptionalValue& GetUpperBound() const {
    if (!!data) {
      return data->GetOptionalValue(RangeBound::kUpper);
    }
    return kNoValue;
  }

  bool IsLowerBoundIncluded() const {
    return !!data && data->IsBoundIncluded(RangeBound::kLower);
  }
  bool IsUpperBoundIncluded() const {
    return !!data && data->IsBoundIncluded(RangeBound::kUpper);
  }

 private:
  template <typename U>
  friend class Range;

  struct RangeData {
    // Unbounded range
    RangeData() noexcept = default;

    template <typename U>
    RangeData(U&& lower, U&& upper, RangeBounds bounds)
        : RangeData{OptionalValue{std::forward<U>(lower)},
                    OptionalValue{std::forward<U>(upper)}, bounds} {}

    template <typename U>
    RangeData(U&& lower, UnboundedType,
              RangeBounds bounds) noexcept(kNothrowValueCopy)
        : RangeData{OptionalValue{std::forward<U>(lower)}, OptionalValue{},
                    bounds} {}

    template <typename U>
    RangeData(UnboundedType, U&& upper,
              RangeBounds bounds) noexcept(kNothrowValueCopy)
        : RangeData{OptionalValue{}, OptionalValue{std::forward<U>(upper)},
                    bounds} {}

    RangeData(OptionalValue low, OptionalValue up, RangeBounds bounds)
        : bounds{bounds}, lower{std::move(low)}, upper{std::move(up)} {
      if (lower && upper && *upper < *lower) {
        throw LogicError("Range lower bound is greater than upper");
      }
    }

    bool operator==(const RangeData& rhs) const {
      return BoundEqual(rhs, RangeBound::kLower) &&
             BoundEqual(rhs, RangeBound::kUpper);
    }

    bool operator!=(const RangeData& rhs) const { return !(*this == rhs); }

    bool HasBound(RangeBounds side) const;

    bool IsBoundIncluded(RangeBounds side) const {
      return HasBound(side) && (bounds & side);
    }

    bool BoundEqual(const RangeData& rhs, RangeBounds side) const;

    // Using this function without checking is ub
    const T& GetBoundValue(RangeBounds side) const {
      if (side == RangeBound::kLower) return *lower;
      UASSERT_MSG(side == RangeBound::kUpper,
                  "Invalid bounds side argument value");
      return *upper;
    }

    const OptionalValue& GetOptionalValue(RangeBounds side) const {
      if (side == RangeBound::kLower) return lower;
      UASSERT_MSG(side == RangeBound::kUpper,
                  "Invalid bounds side argument value");
      return upper;
    }

    RangeBounds bounds = RangeBound::kNone;
    OptionalValue lower;
    OptionalValue upper;
  };

  template <typename U>
  static OptionalValue ConvertBound(const std::optional<U>& rhs) {
    if (!rhs) return OptionalValue{};
    return OptionalValue{*rhs};
  }

  template <typename U>
  static std::optional<RangeData> ConvertData(const Range<U>& rhs) {
    if (!rhs.data) return {};
    return RangeData{ConvertBound(rhs.data->lower),
                     ConvertBound(rhs.data->upper), rhs.data->bounds};
  }

  std::optional<RangeData> data;

  static const inline OptionalValue kNoValue{};
};

template <typename T>
auto MakeRange(T&& lower, T&& upper, RangeBounds bounds = RangeBound::kLower) {
  using ElementType = std::decay_t<T>;
  return Range<ElementType>{std::forward<T>(lower), std::forward<T>(upper),
                            bounds};
}

template <typename T>
auto MakeRange(T&& lower, UnboundedType,
               RangeBounds bounds = RangeBound::kLower) {
  using ElementType = std::decay_t<T>;
  return Range<ElementType>{std::forward<T>(lower), kUnbounded, bounds};
}

template <typename T>
auto MakeRange(UnboundedType, T&& upper,
               RangeBounds bounds = RangeBound::kNone) {
  using ElementType = std::decay_t<T>;
  return Range<ElementType>{kUnbounded, std::forward<T>(upper), bounds};
}

using IntegerRange = Range<Integer>;
using BigintRange = Range<Bigint>;

template <typename T>
class BoundedRange {
  static constexpr bool kNothrowValueCtor =
      std::is_nothrow_default_constructible_v<T>;

 public:
  using ValueType = T;

  BoundedRange() noexcept(kNothrowValueCtor);

  template <typename U, typename = std::enable_if_t<
                            std::is_convertible_v<std::decay_t<U>, T>>>
  BoundedRange(U&& lower, U&& upper, RangeBounds bounds = RangeBound::kLower);

  template <typename U>
  explicit BoundedRange(Range<U>&&);

  bool operator==(const BoundedRange& rhs) const;
  bool operator!=(const BoundedRange& rhs) const { return !(*this == rhs); }

  const ValueType& GetLowerBound() const { return *value_.GetLowerBound(); }
  bool IsLowerBoundIncluded() const { return value_.IsLowerBoundIncluded(); }

  const ValueType& GetUpperBound() const { return *value_.GetUpperBound(); }
  bool IsUpperBoundIncluded() const { return value_.IsUpperBoundIncluded(); }

  const Range<T>& GetUnboundedRange() const { return value_; }

  // TODO Intersection and containment test functions on user demand
 private:
  Range<T> value_;
};

using BoundedIntegerRange = BoundedRange<Integer>;
using BoundedBigintRange = BoundedRange<Bigint>;

}  // namespace storages::postgres

namespace storages::postgres::io {

namespace detail {

enum class RangeFlag {
  kNone = 0x00,
  kEmpty = 0x01,
  kLowerBoundInclusive = 0x02,
  kUpperBoundInclusive = 0x04,
  kLowerBoundInfinity = 0x08,
  kUpperBoundInfinity = 0x10,
  kLowerBoundNull = 0x20,
  kUpperBoundNull = 0x40,
  kContainEmpty = 0x80,
};

using RangeFlags = USERVER_NAMESPACE::utils::Flags<RangeFlag>;

constexpr bool HasLowerBound(RangeFlags flags) {
  return !(flags & RangeFlags{RangeFlag::kEmpty, RangeFlag::kLowerBoundNull,
                              RangeFlag::kLowerBoundInfinity});
}

constexpr bool HasUpperBound(RangeFlags flags) {
  return !(flags & RangeFlags{RangeFlag::kEmpty, RangeFlag::kUpperBoundNull,
                              RangeFlag::kUpperBoundInfinity});
}

template <typename T>
struct RangeBinaryParser : BufferParserBase<Range<T>> {
  using BaseType = BufferParserBase<Range<T>>;
  using ValueType = typename BaseType::ValueType;
  using ElementType = T;
  using ElementParser = typename traits::IO<ElementType>::ParserType;

  static constexpr BufferCategory element_buffer_category =
      traits::kParserBufferCategory<ElementParser>;

  using BaseType::BaseType;

  void operator()(FieldBuffer buffer, const TypeBufferCategory& categories) {
    char wire_range_flags{0};

    buffer.Read(wire_range_flags, BufferCategory::kPlainBuffer);
    RangeFlags range_flags(static_cast<RangeFlag>(wire_range_flags));

    ValueType wire_value;
    if (range_flags != RangeFlag::kEmpty) {
      RangeBounds bounds = RangeBound::kNone;
      typename ValueType::OptionalValue lower;
      typename ValueType::OptionalValue upper;
      if (HasLowerBound(range_flags)) {
        if (range_flags & RangeFlag::kLowerBoundInclusive) {
          bounds |= RangeBound::kLower;
        }
        T tmp;
        buffer.ReadRaw(tmp, categories, element_buffer_category);
        lower = tmp;
      }
      if (HasUpperBound(range_flags)) {
        if (range_flags & RangeFlag::kUpperBoundInclusive) {
          bounds |= RangeBound::kUpper;
        }
        T tmp;
        buffer.ReadRaw(tmp, categories, element_buffer_category);
        upper = tmp;
      }
      wire_value = ValueType{lower, upper, bounds};
    }
    this->value = wire_value;
  }
};

template <typename T>
struct RangeBinaryFormatter : BufferFormatterBase<Range<T>> {
  using BaseType = BufferFormatterBase<Range<T>>;

  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    RangeFlags range_flags;
    if (this->value.Empty()) {
      range_flags |= RangeFlag::kEmpty;
    } else {
      // Mark lower/upper bound
      if (!this->value.HasLowerBound()) {
        range_flags |= RangeFlag::kLowerBoundInfinity;
      } else if (this->value.IsLowerBoundIncluded()) {
        range_flags |= RangeFlag::kLowerBoundInclusive;
      }
      if (!this->value.HasUpperBound()) {
        range_flags |= RangeFlag::kUpperBoundInfinity;
      } else if (this->value.IsUpperBoundIncluded()) {
        range_flags |= RangeFlag::kUpperBoundInclusive;
      }
    }
    char wire_range_flags = static_cast<char>(range_flags.GetValue());
    io::WriteBuffer(types, buffer, wire_range_flags);
    if (!this->value.Empty()) {
      // Write lower/upper bounds
      if (this->value.HasLowerBound()) {
        io::WriteRawBinary(types, buffer, this->value.GetLowerBound());
      }
      if (this->value.HasUpperBound()) {
        io::WriteRawBinary(types, buffer, this->value.GetUpperBound());
      }
    }
  }
};

template <typename T>
struct BoundedRangeBinaryParser : BufferParserBase<BoundedRange<T>> {
  using BaseType = BufferParserBase<BoundedRange<T>>;
  using ValueType = typename BaseType::ValueType;

  using BaseType::BaseType;

  void operator()(FieldBuffer buffer, const TypeBufferCategory& categories) {
    Range<T> tmp;
    io::ReadBuffer(buffer, tmp, categories);
    this->value = ValueType{std::move(tmp)};
  }
};

template <typename T>
struct BoundedRangeBinaryFormatter : BufferFormatterBase<BoundedRange<T>> {
  using BaseType = BufferFormatterBase<BoundedRange<T>>;

  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    io::WriteBuffer(types, buffer, this->value.GetUnboundedRange());
  }
};

}  // namespace detail

namespace traits {

template <typename T>
struct Input<Range<T>, std::enable_if_t<kHasParser<T>>> {
  using type = io::detail::RangeBinaryParser<T>;
};

template <typename T>
struct ParserBufferCategory<io::detail::RangeBinaryParser<T>>
    : std::integral_constant<BufferCategory, BufferCategory::kRangeBuffer> {};

template <typename T>
struct Output<Range<T>, std::enable_if_t<kHasFormatter<T>>> {
  using type = io::detail::RangeBinaryFormatter<T>;
};

template <typename T>
struct Input<BoundedRange<T>, std::enable_if_t<kHasParser<T>>> {
  using type = io::detail::BoundedRangeBinaryParser<T>;
};

template <typename T>
struct ParserBufferCategory<io::detail::BoundedRangeBinaryParser<T>>
    : std::integral_constant<BufferCategory, BufferCategory::kRangeBuffer> {};

template <typename T>
struct Output<BoundedRange<T>, std::enable_if_t<kHasFormatter<T>>> {
  using type = io::detail::BoundedRangeBinaryFormatter<T>;
};

}  // namespace traits

template <>
struct CppToSystemPg<IntegerRange> : PredefinedOid<PredefinedOids::kInt4Range> {
};
template <>
struct CppToSystemPg<BoundedIntegerRange>
    : PredefinedOid<PredefinedOids::kInt4Range> {};
template <>
struct CppToSystemPg<BigintRange> : PredefinedOid<PredefinedOids::kInt8Range> {
};
template <>
struct CppToSystemPg<BoundedBigintRange>
    : PredefinedOid<PredefinedOids::kInt8Range> {};

}  // namespace storages::postgres::io

namespace storages::postgres {

template <typename T>
template <typename U, typename>
Range<T>::Range(U&& lower, U&& upper, RangeBounds bounds)
    : data{RangeData{std::forward<U>(lower), std::forward<U>(upper), bounds}} {
  if (lower == upper && bounds != RangeBound::kBoth) {
    // this will make an empty range
    data.reset();
  }
}

template <typename T>
template <typename U, typename>
Range<T>::Range(U&& lower, UnboundedType ub,
                RangeBounds bounds) noexcept(kNothrowValueCopy)
    : data{RangeData{std::forward<U>(lower), ub, bounds}} {}

template <typename T>
template <typename U, typename>
Range<T>::Range(UnboundedType ub, U&& upper,
                RangeBounds bounds) noexcept(kNothrowValueCopy)
    : data{RangeData{ub, std::forward<U>(upper), bounds}} {}

template <typename T>
Range<T>::Range(const OptionalValue& lower, const OptionalValue& upper,
                RangeBounds bounds)
    : data{RangeData{lower, upper, bounds}} {}

template <typename T>
template <typename U, typename>
Range<T>::Range(const Range<U>& rhs) : data{ConvertData(rhs)} {}

template <typename T>
bool Range<T>::operator==(const Range& rhs) const {
  return (Empty() && rhs.Empty()) || (data == rhs.data);
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const Range<T>& val) {
  if (val.Empty()) return os << "empty";
  if (val.HasLowerBound() && val.IsLowerBoundIncluded())
    os << '[';
  else
    os << '(';
  if (val.HasLowerBound())
    os << *val.GetLowerBound();
  else
    os << "-inf";
  os << ", ";
  if (val.HasUpperBound())
    os << *val.GetUpperBound();
  else
    os << "inf";
  if (val.HasUpperBound() && val.IsUpperBoundIncluded())
    os << ']';
  else
    os << ')';
  return os;
}

template <typename T>
bool Range<T>::RangeData::HasBound(RangeBounds side) const {
  if (side == RangeBound::kLower) {
    return !!lower;
  }
  if (side == RangeBound::kUpper) {
    return !!upper;
  }
  return false;
}

template <typename T>
bool Range<T>::RangeData::BoundEqual(const RangeData& rhs,
                                     RangeBounds side) const {
  bool has_bound = HasBound(side);
  if (has_bound != rhs.HasBound(side)) {
    return false;
  }
  if (!has_bound) {  // both are unbounded
    return true;
  }
  const auto& lval = GetBoundValue(side);
  const auto& rval = rhs.GetBoundValue(side);
  if ((bounds & side) == (rhs.bounds & side)) {
    // same include
    return lval == rval;
  }
  if constexpr (kIsDiscreteValue) {
    T diff = (side == RangeBound::kLower ? 1 : -1);
    if (IsBoundIncluded(side)) {
      return lval == rval + diff;
    } else {
      return lval + diff == rval;
    }
  }
  return false;
}

template <typename T>
BoundedRange<T>::BoundedRange() noexcept(kNothrowValueCtor)
    : value_{T{}, T{}, RangeBound::kBoth} {}

template <typename T>
template <typename U, typename>
BoundedRange<T>::BoundedRange(U&& lower, U&& upper, RangeBounds bounds)
    : value_{std::forward<U>(lower), std::forward<U>(upper), bounds} {}

template <typename T>
template <typename U>
BoundedRange<T>::BoundedRange(Range<U>&& rhs) : value_{std::move(rhs)} {
  if (value_.Empty()) {
    throw BoundedRangeError{"empty range"};
  }
  if (!value_.HasLowerBound()) {
    throw BoundedRangeError{"lower bound is missing"};
  }
  if (!value_.HasUpperBound()) {
    throw BoundedRangeError{"upper bound is missing"};
  }
}

template <typename T>
bool BoundedRange<T>::operator==(const BoundedRange& rhs) const {
  return value_ == rhs.value_;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const BoundedRange<T>& val) {
  return os << val.GetUnboundedRange();
}
}  // namespace storages::postgres

// TODO fmt::format specializations on user demand

USERVER_NAMESPACE_END
