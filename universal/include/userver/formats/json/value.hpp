#pragma once

/// @file userver/formats/json/value.hpp
/// @brief @copybrief formats::json::Value

#include <string_view>
#include <type_traits>

#include <userver/formats/common/items.hpp>
#include <userver/formats/common/meta.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/impl/types.hpp>
#include <userver/formats/json/iterator.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/string_builder_fwd.hpp>
#include <userver/formats/parse/common.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {
class LogHelper;
}  // namespace logging

namespace formats::json {
namespace impl {
class InlineObjectBuilder;
class InlineArrayBuilder;
class MutableValueWrapper;
class StringBuffer;

// do not make a copy of string
impl::Value MakeJsonStringViewValue(std::string_view view);

}  // namespace impl

class ValueBuilder;

namespace parser {
class JsonValueParser;
}  // namespace parser

/// @ingroup userver_containers userver_formats
///
/// @brief Non-mutable JSON value representation.
///
/// Class provides non mutable access JSON value. For modification and
/// construction of new JSON values use formats::json::ValueBuilder.
///
/// ## Example usage:
///
/// @snippet formats/json/value_test.cpp  Sample formats::json::Value usage
///
/// @see @ref md_en_userver_formats
class Value final {
 public:
  struct IterTraits {
    using ValueType = formats::json::Value;
    using Reference = const formats::json::Value&;
    using Pointer = const formats::json::Value*;
    using ContainerType = Value;
  };
  struct DefaultConstructed {};

  using const_iterator =
      Iterator<IterTraits, common::IteratorDirection::kForward>;
  using const_reverse_iterator =
      Iterator<IterTraits, common::IteratorDirection::kReverse>;
  using Exception = formats::json::Exception;
  using ParseException = formats::json::ParseException;
  using Builder = ValueBuilder;

  /// @brief Constructs a Value that holds a null.
  Value() noexcept = default;

  Value(const Value&) = default;
  Value(Value&&) noexcept = default;

  Value& operator=(const Value&) & = default;
  Value& operator=(Value&& other) & noexcept = default;

  template <class T>
  Value& operator=(T&&) && {
    static_assert(!sizeof(T),
                  "You're assigning to a temporary formats::json::Value! Use "
                  "formats::json::ValueBuilder for data modifications.");
    return *this;
  }

  /// @brief Access member by key for read.
  /// @throw TypeMismatchException if not a missing value, an object or null.
  Value operator[](std::string_view key) const;
  /// @brief Access array member by index for read.
  /// @throw TypeMismatchException if not an array value.
  /// @throw OutOfBoundsException if index is greater or equal
  /// than size.
  Value operator[](std::size_t index) const;

  /// @brief Returns an iterator to the beginning of the held array or map.
  /// @throw TypeMismatchException if not an array, object, or null.
  const_iterator begin() const;

  /// @brief Returns an iterator to the end of the held array or map.
  /// @throw TypeMismatchException if not an array, object, or null.
  const_iterator end() const;

  /// @brief Returns an iterator to the reversed begin of the held array.
  /// @throw TypeMismatchException if not an array or null.
  const_reverse_iterator rbegin() const;

  /// @brief Returns an iterator to the reversed end of the held array.
  /// @throw TypeMismatchException if not an array or null.
  const_reverse_iterator rend() const;

  /// @brief Returns whether the array or object is empty.
  /// Returns true for null.
  /// @throw TypeMismatchException if not an array, object, or null.
  bool IsEmpty() const;

  /// @brief Returns array size, object members count, or 0 for null.
  /// @throw TypeMismatchException if not an array, object, or null.
  std::size_t GetSize() const;

  /// @brief Compares values.
  /// @throw MemberMissingException if `*this` or `other` is missing.
  bool operator==(const Value& other) const;
  bool operator!=(const Value& other) const;

  /// @brief Returns true if *this holds nothing. When `IsMissing()` returns
  /// `true` any attempt to get the actual value or iterate over *this will
  bool IsMissing() const noexcept;

  /// @brief Returns true if *this holds a null (Type::kNull).
  bool IsNull() const noexcept;

  /// @brief Returns true if *this holds a bool.
  bool IsBool() const noexcept;

  /// @brief Returns true if *this holds an int.
  bool IsInt() const noexcept;

  /// @brief Returns true if *this holds an int64_t.
  bool IsInt64() const noexcept;

  /// @brief Returns true if *this holds an uint64_t.
  bool IsUInt64() const noexcept;

  /// @brief Returns true if *this holds a double.
  bool IsDouble() const noexcept;

  /// @brief Returns true if *this is holds a std::string.
  bool IsString() const noexcept;

  /// @brief Returns true if *this is holds an array (Type::kArray).
  bool IsArray() const noexcept;

  /// @brief Returns true if *this holds a map (Type::kObject).
  bool IsObject() const noexcept;

  // clang-format off

  /// @brief Returns value of *this converted to T.
  /// @throw Anything derived from std::exception.
  ///
  /// ## Example usage:
  ///
  /// @snippet formats/json/value_test.cpp  Sample formats::json::Value::As<T>() usage
  ///
  /// @see @ref md_en_userver_formats

  // clang-format on

  template <typename T>
  T As() const;

  /// @brief Returns value of *this converted to T or T(args) if
  /// this->IsMissing().
  /// @throw Anything derived from std::exception.
  template <typename T, typename First, typename... Rest>
  T As(First&& default_arg, Rest&&... more_default_args) const;

  /// @brief Returns value of *this converted to T or T() if this->IsMissing().
  /// @throw Anything derived from std::exception.
  /// @note Use as `value.As<T>({})`
  template <typename T>
  T As(DefaultConstructed) const;

  /// @brief Extracts the specified type with relaxed type checks.
  /// For example, `true` may be converted to 1.0.
  template <typename T>
  T ConvertTo() const;

  /// Extracts the specified type with strict type checks, or constructs the
  /// default value when the field is not present
  template <typename T, typename First, typename... Rest>
  T ConvertTo(First&& default_arg, Rest&&... more_default_args) const;

  /// @brief Returns true if *this holds a `key`.
  /// @throw TypeMismatchException if `*this` is not a map or null.
  bool HasMember(std::string_view key) const;

  /// @brief Returns full path to this value.
  std::string GetPath() const;

  /// @brief Returns new value that is an exact copy if the existing one
  /// but references different memory (a deep copy of a *this). The returned
  /// value is a root value with path '/'.
  /// @throws MemberMissingException id `this->IsMissing()`.
  Value Clone() const;

  /// @throw MemberMissingException if `this->IsMissing()`.
  void CheckNotMissing() const;

  /// @throw MemberMissingException if `*this` is not an array or null.
  void CheckArrayOrNull() const;

  /// @throw TypeMismatchException if `*this` is not a map or null.
  void CheckObjectOrNull() const;

  /// @throw TypeMismatchException if `*this` is not a map.
  void CheckObject() const;

  /// @throw TypeMismatchException if `*this` is not a map, array or null.
  void CheckObjectOrArrayOrNull() const;

  /// @throw TypeMismatchException if `*this` is not a map, array or null;
  /// `OutOfBoundsException` if `index >= this->GetSize()`.
  void CheckInBounds(std::size_t index) const;

  /// @brief Returns true if *this is a first (root) value.
  bool IsRoot() const noexcept;

  /// @brief Returns true if `*this` and `other` reference the value by the same
  /// pointer.
  bool DebugIsReferencingSameMemory(const Value& other) const {
    return value_ptr_ == other.value_ptr_;
  }

 private:
  class EmplaceEnabler {};

 public:
  /// @cond
  Value(EmplaceEnabler, const impl::VersionedValuePtr& root,
        const impl::Value& value, int depth);
  /// @endcond

 private:
  explicit Value(impl::VersionedValuePtr root) noexcept;
  Value(impl::VersionedValuePtr root, const impl::Value* value_ptr, int depth);
  Value(impl::VersionedValuePtr root, std::string&& detached_path);

  bool IsUniqueReference() const;
  void EnsureNotMissing();
  const impl::Value& GetNative() const;
  impl::Value& GetNative();
  void SetNative(impl::Value&);  // does not copy
  int GetExtendedType() const;

  impl::VersionedValuePtr root_;
  impl::Value* value_ptr_{nullptr};
  /// Full path of node (only for missing nodes)
  std::string detached_path_;
  /// Depth of the node to ease recursive traversal in GetPath()
  int depth_{0};

  template <typename, common::IteratorDirection>
  friend class Iterator;
  friend class ValueBuilder;
  friend class StringBuilder;
  friend class impl::InlineObjectBuilder;
  friend class impl::InlineArrayBuilder;
  friend class impl::MutableValueWrapper;
  friend class parser::JsonValueParser;
  friend class impl::StringBuffer;

  friend formats::json::Value FromString(std::string_view);
  friend formats::json::Value FromStream(std::istream&);
  friend void Serialize(const formats::json::Value&, std::ostream&);
  friend std::string ToString(const formats::json::Value&);
  friend std::string ToStableString(const formats::json::Value&);
  friend std::string ToStableString(formats::json::Value&&);
  friend logging::LogHelper& operator<<(logging::LogHelper&, const Value&);
};

template <typename T>
T Value::As() const {
  static_assert(formats::common::impl::kHasParse<Value, T>,
                "There is no `Parse(const Value&, formats::parse::To<T>)` "
                "in namespace of `T` or `formats::parse`. "
                "Probably you forgot to include the "
                "<userver/formats/parse/common_containers.hpp> or you "
                "have not provided a `Parse` function overload.");

  return Parse(*this, formats::parse::To<T>{});
}

template <>
bool Value::As<bool>() const;

template <>
int64_t Value::As<int64_t>() const;

template <>
uint64_t Value::As<uint64_t>() const;

template <>
double Value::As<double>() const;

template <>
std::string Value::As<std::string>() const;

template <>
bool Value::ConvertTo<bool>() const;

template <>
int64_t Value::ConvertTo<int64_t>() const;

template <>
uint64_t Value::ConvertTo<uint64_t>() const;

template <>
double Value::ConvertTo<double>() const;

template <>
std::string Value::ConvertTo<std::string>() const;

template <typename T, typename First, typename... Rest>
T Value::As(First&& default_arg, Rest&&... more_default_args) const {
  if (IsMissing() || IsNull()) {
    // intended raw ctor call, sometimes casts
    // NOLINTNEXTLINE(google-readability-casting)
    return T(std::forward<First>(default_arg),
             std::forward<Rest>(more_default_args)...);
  }
  return As<T>();
}

template <typename T>
T Value::As(Value::DefaultConstructed) const {
  return (IsMissing() || IsNull()) ? T() : As<T>();
}

template <typename T>
T Value::ConvertTo() const {
  if constexpr (formats::common::impl::kHasConvert<Value, T>) {
    return Convert(*this, formats::parse::To<T>{});
  } else if constexpr (formats::common::impl::kHasParse<Value, T>) {
    return Parse(*this, formats::parse::To<T>{});
  } else {
    static_assert(
        !sizeof(T),
        "There is no `Convert(const Value&, formats::parse::To<T>)` or"
        "`Parse(const Value&, formats::parse::To<T>)`"
        "in namespace of `T` or `formats::parse`. "
        "Probably you have not provided a `Convert` function overload.");
  }
}

template <typename T, typename First, typename... Rest>
T Value::ConvertTo(First&& default_arg, Rest&&... more_default_args) const {
  if (IsMissing() || IsNull()) {
    // NOLINTNEXTLINE(google-readability-casting)
    return T(std::forward<First>(default_arg),
             std::forward<Rest>(more_default_args)...);
  }
  return ConvertTo<T>();
}

inline Value Parse(const Value& value, parse::To<Value>) { return value; }

/// @brief Wrapper for handy python-like iteration over a map
///
/// @code
///   for (const auto& [name, value]: Items(map)) ...
/// @endcode
using formats::common::Items;

}  // namespace formats::json

/// Although we provide user defined literals, please beware that
/// 'using namespace ABC' may contradict code style of your company.
namespace formats::literals {

json::Value operator"" _json(const char* str, std::size_t len);

}  // namespace formats::literals

USERVER_NAMESPACE_END
