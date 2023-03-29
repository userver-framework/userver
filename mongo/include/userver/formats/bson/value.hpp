#pragma once

/// @file userver/formats/bson/value.hpp
/// @brief @copybrief formats::bson::Value

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>

#include <userver/formats/bson/exception.hpp>
#include <userver/formats/bson/iterator.hpp>
#include <userver/formats/bson/types.hpp>
#include <userver/formats/common/items.hpp>
#include <userver/formats/common/meta.hpp>
#include <userver/formats/parse/common.hpp>
#include <userver/formats/parse/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::bson {
namespace impl {
class BsonBuilder;
class ValueImpl;
}  // namespace impl

class Document;
class ValueBuilder;

/// @brief Non-mutable BSON value representation.
///
/// Class provides non mutable access BSON value. For modification and
/// construction of new BSON values use formats::bson::ValueBuilder.
///
/// ## Example usage:
///
/// @snippet formats/bson/value_test.cpp  Sample formats::bson::Value usage
///
/// @see @ref md_en_userver_formats
class Value {
 public:
  struct DefaultConstructed {};

  using const_iterator =
      Iterator<const Value, common::IteratorDirection::kForward>;
  using const_reverse_iterator =
      Iterator<const Value, common::IteratorDirection::kReverse>;
  using Exception = formats::bson::BsonException;
  using ParseException = formats::bson::ConversionException;
  using Builder = ValueBuilder;

  /// @brief Selectors for duplicate fields parsing behavior
  /// @see SetDuplicateFieldsPolicy
  enum class DuplicateFieldsPolicy { kForbid, kUseFirst, kUseLast };

  /// Constructs a `null` value
  Value();

  Value(const Value&) = default;
  Value(Value&&) noexcept = default;
  Value& operator=(const Value&) & = default;
  Value& operator=(Value&&) & noexcept = default;

  template <class T>
  Value& operator=(T&&) && {
    static_assert(!sizeof(T),
                  "You're assigning to a temporary formats::bson::Value! Use "
                  "formats::bson::ValueBuilder for data modifications.");
    return *this;
  }

  /// @cond
  /// Constructor from implementation, internal use only
  explicit Value(impl::ValueImplPtr);
  /// @endcond

  /// @brief Retrieves document field by name
  /// @param name field name
  /// @throws TypeMismatchException if value is not a missing value, a document,
  /// or `null`
  Value operator[](const std::string& name) const;

  /// @brief Retrieves array element by index
  /// @param index element index
  /// @throws TypeMismatchException if value is not an array or `null`
  /// @throws OutOfBoundsException if index is invalid for the array
  Value operator[](uint32_t index) const;

  /// @brief Checks whether the document has a field
  /// @param name field name
  /// @throws TypeMismatchExcepiton if value is not a document or `null`
  bool HasMember(const std::string& name) const;

  /// @brief Returns an iterator to the first array element/document field
  /// @throws TypeMismatchException if value is not a document, array or `null`
  const_iterator begin() const;

  /// @brief Returns an iterator following the last array element/document field
  /// @throws TypeMismatchException if value is not a document, array or `null`
  const_iterator end() const;

  /// @brief Returns a reversed iterator to the last array element
  /// @throws TypeMismatchException if value is not an array or `null`
  const_reverse_iterator rbegin() const;

  /// @brief Returns a reversed iterator following the first array element
  /// @throws TypeMismatchException if value is not an array or `null`
  const_reverse_iterator rend() const;

  /// @brief Returns whether the document/array is empty
  /// @throws TypeMismatchException if value is not a document, array or `null`
  /// @note Returns `true` for `null`.
  bool IsEmpty() const;

  /// @brief Returns the number of elements in a document/array
  /// @throws TypeMismatchException if value is not a document, array or `null`
  /// @note May require linear time before the first element access.
  /// @note Returns 0 for `null`.
  uint32_t GetSize() const;

  /// Returns value path in a document
  std::string GetPath() const;

  bool operator==(const Value&) const;
  bool operator!=(const Value&) const;

  /// @brief Checks whether the selected element exists
  /// @note MemberMissingException is throws on nonexisting element access
  bool IsMissing() const;

  /// @name Type checking
  /// @{
  bool IsArray() const;
  bool IsDocument() const;
  bool IsNull() const;
  bool IsBool() const;
  bool IsInt32() const;
  bool IsInt64() const;
  bool IsDouble() const;
  bool IsString() const;
  bool IsDateTime() const;
  bool IsOid() const;
  bool IsBinary() const;
  bool IsDecimal128() const;
  bool IsMinKey() const;
  bool IsMaxKey() const;
  bool IsTimestamp() const;

  bool IsObject() const { return IsDocument(); }
  /// @}

  // clang-format off

  /// Extracts the specified type with strict type checks
  ///
  /// ## Example usage:
  ///
  /// @snippet formats/bson/value_test.cpp  Sample formats::bson::Value::As<T>() usage
  ///
  /// @see @ref md_en_userver_formats

  // clang-format on

  template <typename T>
  T As() const {
    static_assert(
        formats::common::impl::kHasParse<Value, T>,
        "There is no `Parse(const Value&, formats::parse::To<T>)` in namespace "
        "of `T` or `formats::parse`. "
        "Probably you have not provided a `Parse` function overload.");

    return Parse(*this, formats::parse::To<T>{});
  }

  /// Extracts the specified type with strict type checks, or constructs the
  /// default value when the field is not present
  template <typename T, typename First, typename... Rest>
  T As(First&& default_arg, Rest&&... more_default_args) const {
    if (IsMissing() || IsNull()) {
      // intended raw ctor call, sometimes casts
      // NOLINTNEXTLINE(google-readability-casting)
      return T(std::forward<First>(default_arg),
               std::forward<Rest>(more_default_args)...);
    }
    return As<T>();
  }

  /// @brief Returns value of *this converted to T or T() if this->IsMissing().
  /// @throw Anything derived from std::exception.
  /// @note Use as `value.As<T>({})`
  template <typename T>
  T As(DefaultConstructed) const {
    return (IsMissing() || IsNull()) ? T() : As<T>();
  }

  /// @brief Extracts the specified type with relaxed type checks.
  /// For example, `true` may be converted to 1.0.
  template <typename T>
  T ConvertTo() const {
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

  /// Extracts the specified type with strict type checks, or constructs the
  /// default value when the field is not present
  template <typename T, typename First, typename... Rest>
  T ConvertTo(First&& default_arg, Rest&&... more_default_args) const {
    if (IsMissing() || IsNull()) {
      // NOLINTNEXTLINE(google-readability-casting)
      return T(std::forward<First>(default_arg),
               std::forward<Rest>(more_default_args)...);
    }
    return ConvertTo<T>();
  }

  /// @brief Changes parsing behavior when duplicate fields are encountered.
  /// Should not be used normally.
  /// @details Should be called before the first field access. Only affects
  /// documents. Default policy is to throw an exception when duplicate fields
  /// are encountered.
  /// @warning At most one value will be read, all others will be discarded and
  /// cannot be serialized back!
  void SetDuplicateFieldsPolicy(DuplicateFieldsPolicy);

  /// Throws a MemberMissingException if the selected element does not exist
  void CheckNotMissing() const;

  /// @brief Throws a TypeMismatchException if the selected element
  /// is not an array or null
  void CheckArrayOrNull() const;

  /// @brief Throws a TypeMismatchException if the selected element
  /// is not a document or null
  void CheckDocumentOrNull() const;

  /// @cond
  /// Same, for parsing capabilities
  void CheckObjectOrNull() const { CheckDocumentOrNull(); }

  /// @brief Returns an array as its internal representation (BSON document),
  /// internal use only
  Document GetInternalArrayDocument() const;
  /// @endcond

 protected:
  const impl::BsonHolder& GetBson() const;

 private:
  friend class ValueBuilder;
  friend class impl::BsonBuilder;

  impl::ValueImplPtr impl_;
};

/// @cond
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
std::chrono::system_clock::time_point
Value::As<std::chrono::system_clock::time_point>() const;

template <>
Oid Value::As<Oid>() const;

template <>
Binary Value::As<Binary>() const;

template <>
Decimal128 Value::As<Decimal128>() const;

template <>
Timestamp Value::As<Timestamp>() const;

template <>
Document Value::As<Document>() const;

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
/// @endcond

/// @brief Wrapper for handy python-like iteration over a map
///
/// @code
///   for (const auto& [name, value]: Items(map)) ...
/// @endcode
using formats::common::Items;

}  // namespace formats::bson

/// Although we provide user defined literals, please beware that
/// 'using namespace ABC' may contradict code style of your company.
namespace formats::literals {

bson::Value operator"" _bson(const char* str, std::size_t len);

}  // namespace formats::literals

USERVER_NAMESPACE_END
