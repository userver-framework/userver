#pragma once

/// @file storages/postgres/io/range_type.hpp
/// @brief Ranges I/O support

#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/io/buffer_io_base.hpp>
#include <storages/postgres/io/field_buffer.hpp>
#include <storages/postgres/io/traits.hpp>
#include <storages/postgres/io/type_mapping.hpp>
#include <storages/postgres/io/type_traits.hpp>
#include <storages/postgres/io/user_types.hpp>

#include <utils/assert.hpp>
#include <utils/flags.hpp>

#include <iostream>

namespace storages::postgres {

struct UnboundedType {};
constexpr UnboundedType kUnbounded{};

enum class RangeBound {
  kNone = 0x00,
  kLower = 0x01,
  kUpper = 0x02,
  kBoth = kLower | kUpper,
};

using RangeBounds = ::utils::Flags<RangeBound>;

template <typename T>
class Range {
  static constexpr bool kNothrowValueCtor =
      std::is_nothrow_default_constructible<T>::value;
  static constexpr bool kNothrowValueCopy =
      std::is_nothrow_copy_constructible<T>::value;
  static constexpr bool kNothrowValueMove =
      std::is_nothrow_move_constructible<T>::value;
  static constexpr bool kIsDiscreteValue = std::is_integral<T>::value;

 public:
  using OptionalValue = boost::optional<T>;

  /// Empty range
  Range() noexcept {}

  /// Unbounded range
  Range(UnboundedType, UnboundedType) noexcept : data{RangeData{}} {}

  /// Bounded range
  template <typename U, typename = std::enable_if_t<std::is_same<T, U>::value>>
  Range(U&& lower, U&& upper,
        RangeBounds bounds = RangeBound::kLower) noexcept(kNothrowValueCopy);

  /// Range with a lower bound
  template <typename U, typename = std::enable_if_t<std::is_same<T, U>::value>>
  Range(U&& lower, UnboundedType ub,
        RangeBounds bounds = RangeBound::kLower) noexcept(kNothrowValueCopy);

  /// Range with an upper bound
  template <typename U, typename = std::enable_if_t<std::is_same<T, U>::value>>
  Range(UnboundedType ub, U&& upper,
        RangeBounds bounds = RangeBound::kNone) noexcept(kNothrowValueCopy);

  Range(const OptionalValue& lower, const OptionalValue& upper,
        RangeBounds bounds) noexcept(kNothrowValueCopy);

  /// Convert from a range of different type.
  ///
  /// Intentionally implicit
  template <typename U,
            typename = std::enable_if_t<std::is_convertible<U, T>::value>>
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
    RangeData(U&& lower, U&& upper,
              RangeBounds bounds) noexcept(kNothrowValueCopy)
        : bounds{bounds},
          lower{std::forward<U>(lower)},
          upper{std::forward<U>(upper)} {
      if (lower > upper) {
        throw LogicError("Range lower bound is greater than upper");
      }
    }

    template <typename U>
    RangeData(U&& lower, UnboundedType,
              RangeBounds bounds) noexcept(kNothrowValueCopy)
        : bounds{bounds}, lower{std::forward<U>(lower)}, upper{} {}

    template <typename U>
    RangeData(UnboundedType, U&& upper,
              RangeBounds bounds) noexcept(kNothrowValueCopy)
        : bounds{bounds}, lower{}, upper{std::forward<U>(upper)} {}

    RangeData(OptionalValue lower, OptionalValue upper, RangeBounds bounds)
        : bounds{bounds}, lower{lower}, upper{upper} {}

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
      if (side == RangeBound::kUpper) return *upper;
      UASSERT_MSG(false, "Invalid bounds side argument value");
    }

    const OptionalValue& GetOptionalValue(RangeBounds side) const {
      if (side == RangeBound::kLower) return lower;
      if (side == RangeBound::kUpper) return upper;
      UASSERT_MSG(false, "Invalid bounds side argument value");
    }

    RangeBounds bounds = RangeBound::kNone;
    OptionalValue lower;
    OptionalValue upper;
  };

  template <typename U>
  static OptionalValue ConvertBound(const boost::optional<U>& rhs) {
    if (!rhs) return OptionalValue{};
    return OptionalValue{*rhs};
  }

  template <typename U>
  static boost::optional<RangeData> ConvertData(const Range<U>& rhs) {
    if (!rhs.data) return {};
    return RangeData{ConvertBound(rhs.data->lower),
                     ConvertBound(rhs.data->upper), rhs.data->bounds};
  }

  boost::optional<RangeData> data;

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

}  // namespace storages::postgres

namespace storages::postgres::io {
/// @page pg_range_types µPg: Range types
///

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

using RangeFlags = ::utils::Flags<RangeFlag>;

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
  using ElementParser =
      typename traits::IO<ElementType,
                          DataFormat::kBinaryDataFormat>::ParserType;

  static constexpr BufferCategory element_buffer_category =
      traits::kParserBufferCategory<ElementParser>;

  using BaseType::BaseType;

  void operator()(FieldBuffer buffer,
                  [[maybe_unused]] const TypeBufferCategory& categories) {
    std::size_t offset{0};
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
  void operator()(const UserTypes& types,
                  [[maybe_unused]] Buffer& buffer) const {
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
    WriteBinary(types, buffer, wire_range_flags);
    if (!this->value.Empty()) {
      // Write lower/upper bounds
      if (this->value.HasLowerBound()) {
        WriteRawBinary(types, buffer, this->value.GetLowerBound());
      }
      if (this->value.HasUpperBound()) {
        WriteRawBinary(types, buffer, this->value.GetUpperBound());
      }
    }
  }
};

}  // namespace detail

namespace traits {

template <typename T>
struct Input<Range<T>, DataFormat::kBinaryDataFormat,
             std::enable_if_t<kHasBinaryParser<T>>> {
  using type = io::detail::RangeBinaryParser<T>;
};

template <typename T>
struct ParserBufferCategory<io::detail::RangeBinaryParser<T>>
    : std::integral_constant<BufferCategory, BufferCategory::kRangeBuffer> {};

template <typename T>
struct Output<Range<T>, DataFormat::kBinaryDataFormat,
              std::enable_if_t<kHasBinaryFormatter<T>>> {
  using type = io::detail::RangeBinaryFormatter<T>;
};

}  // namespace traits

template <>
struct CppToSystemPg<IntegerRange> : PredefinedOid<PredefinedOids::kInt4Range> {
};
template <>
struct CppToSystemPg<BigintRange> : PredefinedOid<PredefinedOids::kInt8Range> {
};

}  // namespace storages::postgres::io

namespace storages::postgres {

template <typename T>
template <typename U, typename>
Range<T>::Range(U&& lower, U&& upper,
                RangeBounds bounds) noexcept(kNothrowValueCopy)
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
                RangeBounds bounds) noexcept(kNothrowValueCopy)
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

}  // namespace storages::postgres
