#pragma once

/// @file formats/bson/value.hpp
/// @brief @copybrief formats::bson::Value

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>

#include <formats/bson/iterator.hpp>
#include <formats/bson/types.hpp>

namespace formats::bson {
namespace impl {
class BsonBuilder;
class ValueImpl;
}  // namespace impl

class Document;
class ValueBuilder;

/// BSON value
class Value {
 public:
  using const_iterator = Iterator<Value>;

  /// Constructs a `null` value
  Value();

  /// @cond
  /// Constructor from implementation, internal use only
  explicit Value(impl::ValueImplPtr);
  /// @endcond

  /// @brief Retrieves document field by name
  /// @param name field name
  /// @throws TypeMismatchException if value is not a document or `null`
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

  bool IsObject() const { return IsDocument(); }
  /// @}

  /// Extracts the specified type with strict type checks
  template <typename T>
  T As() const {
    CheckNotMissing();
    const T* dont_use_me = nullptr;
    return ParseBson(*this, dont_use_me);
  }

  /// Extracts the specified type with strict type checks, or constructs the
  /// default value when the field is not present
  template <typename T, typename First, typename... Rest>
  T As(First&& default_arg, Rest&&... more_default_args) const {
    if (IsMissing()) {
      return T(std::forward<First>(default_arg),
               std::forward<Rest>(more_default_args)...);
    }
    return As<T>();
  }

  /// @brief Extracts the specified type with relaxed type checks.
  /// For example, `true` may be converted to 1.0.
  template <typename T>
  T Convert() const {
    const T* dont_use_me = nullptr;
    return ConvertBson(*this, dont_use_me);
  }

  /// Extracts the specified type with strict type checks, or constructs the
  /// default value when the field is not present
  template <typename T, typename First, typename... Rest>
  T Convert(First&& default_arg, Rest&&... more_default_args) const {
    if (IsMissing()) {
      return T(std::forward<First>(default_arg),
               std::forward<Rest>(more_default_args)...);
    }
    return Convert<T>();
  }

  /// Throws a MemberMissingException if the selected element does not exist
  void CheckNotMissing() const;

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
int32_t Value::As<int32_t>() const;

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
Document Value::As<Document>() const;

template <>
bool Value::Convert<bool>() const;

template <>
int32_t Value::Convert<int32_t>() const;

template <>
int64_t Value::Convert<int64_t>() const;

template <>
uint64_t Value::Convert<uint64_t>() const;

template <>
double Value::Convert<double>() const;

template <>
std::string Value::Convert<std::string>() const;

#ifdef _LIBCPP_VERSION
template <>
long Value::As<long>() const;

template <>
unsigned long Value::As<unsigned long>() const;

template <>
long Value::Convert<long>() const;

template <>
unsigned long Value::Convert<unsigned long>() const;
#else
template <>
long long Value::As<long long>() const;

template <>
unsigned long long Value::As<unsigned long long>() const;

template <>
long long Value::Convert<long long>() const;

template <>
unsigned long long Value::Convert<unsigned long long>() const;
#endif
/// @endcond

}  // namespace formats::bson
