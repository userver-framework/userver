#pragma once

/// @file userver/formats/bson/value_builder.hpp
/// @brief @copybrief formats::bson::ValueBuilder

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include <userver/formats/bson/document.hpp>
#include <userver/formats/bson/iterator.hpp>
#include <userver/formats/bson/types.hpp>
#include <userver/formats/bson/value.hpp>
#include <userver/formats/common/meta.hpp>
#include <userver/formats/common/transfer_tag.hpp>
#include <userver/formats/common/type.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::bson {

// clang-format off

/// @brief Builder for BSON.
///
/// Class provides methods for building BSON. For read only access to the
/// existing BSON values use formats::bson::Value.
///
/// ## Example usage:
///
/// @snippet formats/bson/value_builder_test.cpp  Sample formats::bson::ValueBuilder usage
///
/// ## Customization example:
///
/// @snippet formats/bson/value_builder_test.cpp  Sample Customization formats::bson::ValueBuilder usage
///
/// @see @ref md_en_userver_formats

// clang-format on

class ValueBuilder {
 public:
  using iterator = Iterator<ValueBuilder>;
  using Type = formats::common::Type;

  /// Constructs a `null` value (may be used as either document or array)
  ValueBuilder();

  /// Constructs a value with the predefined type
  explicit ValueBuilder(Type);

  /// @cond
  /// Constructor from implementation, internal use only
  explicit ValueBuilder(impl::ValueImplPtr);
  /// @endcond

  ValueBuilder(const ValueBuilder& other);
  // NOLINTNEXTLINE(performance-noexcept-move-constructor)
  ValueBuilder(ValueBuilder&& other);
  ValueBuilder& operator=(const ValueBuilder& other);
  // NOLINTNEXTLINE(performance-noexcept-move-constructor)
  ValueBuilder& operator=(ValueBuilder&& other);

  /// @{
  /// Efficiently constructs a copy of an existing value
  ValueBuilder(const Value&);
  ValueBuilder(const Document&);
  // NOLINTNEXTLINE(performance-noexcept-move-constructor)
  ValueBuilder(Value&&);
  // NOLINTNEXTLINE(performance-noexcept-move-constructor)
  ValueBuilder(Document&&);
  /// @}

  /// @name Concrete type constructors
  /// @{
  /* implicit */ ValueBuilder(std::nullptr_t);
  /* implicit */ ValueBuilder(bool);
  /* implicit */ ValueBuilder(int);
  /* implicit */ ValueBuilder(unsigned int);
  /* implicit */ ValueBuilder(long);
  /* implicit */ ValueBuilder(unsigned long);
  /* implicit */ ValueBuilder(long long);
  /* implicit */ ValueBuilder(unsigned long long);
  /* implicit */ ValueBuilder(double);
  /* implicit */ ValueBuilder(const char*);
  /* implicit */ ValueBuilder(std::string);
  /* implicit */ ValueBuilder(std::string_view);
  /* implicit */ ValueBuilder(const std::chrono::system_clock::time_point&);
  /* implicit */ ValueBuilder(const Oid&);
  /* implicit */ ValueBuilder(Binary);
  /* implicit */ ValueBuilder(const Decimal128&);
  /* implicit */ ValueBuilder(MinKey);
  /* implicit */ ValueBuilder(MaxKey);
  /* implicit */ ValueBuilder(const Timestamp&);
  /// @}

  /// Universal constructor using Serialize
  template <typename T>
  ValueBuilder(const T& t) : ValueBuilder(DoSerialize(t)) {}

  /// @brief Transfers the `ValueBuilder` object
  /// @see formats::common::TransferTag for the transfer semantics
  ValueBuilder(common::TransferTag, ValueBuilder&&) noexcept;

  /// @brief Retrieves or creates document field by name
  /// @throws TypeMismatchException if value is not a document or `null`
  ValueBuilder operator[](const std::string& name);

  /// @brief Access member by key for modification.
  /// @throw `TypeMismatchException` if not object or null value.
  template <
      typename Tag, utils::StrongTypedefOps Ops,
      typename Enable = std::enable_if_t<utils::IsStrongTypedefLoggable(Ops)>>
  ValueBuilder operator[](
      const utils::StrongTypedef<Tag, std::string, Ops>& name);

  /// @brief Retrieves array element by index
  /// @throws TypeMismatchException if value is not an array or `null`
  /// @throws OutOfBoundsException if index is invalid for the array
  ValueBuilder operator[](uint32_t index);

  /// @brief Remove key from object. If key is missing nothing happens.
  /// @throw `TypeMismatchException` if value is not an object.
  void Remove(const std::string& key);

  /// @brief Returns an iterator to the first array element/document field
  /// @throws TypeMismatchException if value is not a document, array or `null`
  iterator begin();

  /// @brief Returns an iterator following the last array element/document field
  /// @throws TypeMismatchException if value is not a document, array or `null`
  iterator end();

  /// @brief Returns whether the document/array is empty
  /// @throws TypeMismatchException if value is not a document, array or `null`
  /// @note Returns `true` for `null`.
  bool IsEmpty() const;

  /// @brief Returns true if *this holds a Null (Type::kNull).
  bool IsNull() const noexcept;

  /// @brief Returns true if *this is convertible to bool.
  bool IsBool() const noexcept;

  /// @brief Returns true if *this is convertible to int.
  bool IsInt() const noexcept;

  /// @brief Returns true if *this is convertible to int64_t.
  bool IsInt64() const noexcept;

  /// @brief Returns true if *this is convertible to uint64_t.
  bool IsUInt64() const noexcept;

  /// @brief Returns true if *this is convertible to double.
  bool IsDouble() const noexcept;

  /// @brief Returns true if *this is convertible to std::string.
  bool IsString() const noexcept;

  /// @brief Returns true if *this is an array (Type::kArray).
  bool IsArray() const noexcept;

  /// @brief Returns true if *this holds a document (BSON_TYPE_DOCUMENT).
  bool IsObject() const noexcept;

  /// @brief Returns the number of elements in a document/array
  /// @throws TypeMismatchException if value is not a document, array or `null`
  /// @note Returns 0 for `null`.
  uint32_t GetSize() const;

  /// @brief Checks whether the document has a field
  /// @param name field name
  /// @throws TypeMismatchExcepiton if value is not a document or `null`
  bool HasMember(const std::string& name) const;

  /// @brief Creates or resizes the array
  /// @param size new size
  /// @throws TypeMismatchException if value is not an array or `null`
  void Resize(uint32_t size);

  /// @brief Appends an element to the array, possibly creating one
  /// @param elem element to append
  /// @throws TypeMismatchException if value is not an array or `null`
  void PushBack(ValueBuilder&& elem);

  /// @brief Retrieves a compiled value from the builder.
  /// After calling this method the builder is in unspecified state.
  Value ExtractValue();

 private:
  void Assign(const impl::ValueImplPtr&);
  void Assign(impl::ValueImplPtr&&);

  template <typename T>
  static Value DoSerialize(const T& t);

  impl::ValueImplPtr impl_;
};

template <typename Tag, utils::StrongTypedefOps Ops, typename Enable>
ValueBuilder ValueBuilder::operator[](
    const utils::StrongTypedef<Tag, std::string, Ops>& name) {
  return (*this)[name.GetUnderlying()];
}

template <typename T>
Value ValueBuilder::DoSerialize(const T& t) {
  static_assert(
      formats::common::impl::kHasSerialize<Value, T>,
      "There is no `Serialize(const T&, formats::serialize::To<bson::Value>)` "
      "in namespace of `T` or `formats::serialize`. "
      ""
      "Probably you forgot to include the "
      "<userver/formats/serialize/common_containers.hpp> or you "
      "have not provided a `Serialize` function overload.");

  return Serialize(t, formats::serialize::To<Value>());
}

}  // namespace formats::bson

USERVER_NAMESPACE_END
