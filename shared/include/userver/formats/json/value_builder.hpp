#pragma once

/// @file userver/formats/json/value_builder.hpp
/// @brief @copybrief formats::json::ValueBuilder

#include <chrono>
#include <string_view>
#include <vector>

#include <userver/formats/common/meta.hpp>
#include <userver/formats/common/transfer_tag.hpp>
#include <userver/formats/json/impl/mutable_value_wrapper.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {

// clang-format off

/// @ingroup userver_containers userver_formats
///
/// @brief Builder for JSON.
///
/// Class provides methods for building JSON. For read only access to the
/// existing JSON values use formats::json::Value.
///
/// ## Example usage:
///
/// @snippet formats/json/value_builder_test.cpp  Sample formats::json::ValueBuilder usage
///
/// ## Customization example:
///
/// @snippet formats/json/value_builder_test.cpp  Sample Customization formats::json::ValueBuilder usage
///
/// @see @ref md_en_userver_formats

// clang-format on

class ValueBuilder final {
 public:
  struct IterTraits {
    using ValueType = formats::json::ValueBuilder;
    using Reference = formats::json::ValueBuilder&;
    using Pointer = formats::json::ValueBuilder*;
    using ContainerType = impl::MutableValueWrapper;
  };

  using iterator = Iterator<IterTraits>;

  /// Constructs a ValueBuilder that holds kNull
  ValueBuilder() noexcept = default;

  /// Constructs a valueBuilder that holds default value for provided `type`.
  ValueBuilder(Type type);

  /// @brief Transfers the `ValueBuilder` object
  /// @see formats::common::TransferTag for the transfer semantics
  ValueBuilder(common::TransferTag, ValueBuilder&&) noexcept;

  ValueBuilder(const ValueBuilder& other);
  // NOLINTNEXTLINE(performance-noexcept-move-constructor)
  ValueBuilder(ValueBuilder&& other);
  ValueBuilder& operator=(const ValueBuilder& other);
  // NOLINTNEXTLINE(performance-noexcept-move-constructor)
  ValueBuilder& operator=(ValueBuilder&& other);

  ValueBuilder(const formats::json::Value& other);
  ValueBuilder(formats::json::Value&& other);

  /// Converting constructors.
  ValueBuilder(bool t);
  ValueBuilder(const char* str);
  ValueBuilder(const std::string& str);
  ValueBuilder(std::string_view str);
  ValueBuilder(int t);
  ValueBuilder(unsigned int t);
  ValueBuilder(uint64_t t);
  ValueBuilder(int64_t t);
  ValueBuilder(float t);
  ValueBuilder(double t);

  /// Universal constructor using Serialize
  template <typename T>
  ValueBuilder(const T& t) : ValueBuilder(DoSerialize(t)) {}

  /// @brief Access member by key for modification.
  /// @throw `TypeMismatchException` if not object or null value.
  ValueBuilder operator[](std::string key);
  /// @brief Access array member by index for modification.
  /// @throw `TypeMismatchException` if not an array value.
  /// @throw `OutOfBoundsException` if index is greater than size.
  ValueBuilder operator[](std::size_t index);
  /// @brief Access member by key for modification.
  /// @throw `TypeMismatchException` if not object or null value.
  template <
      typename Tag, utils::StrongTypedefOps Ops,
      typename Enable = std::enable_if_t<utils::IsStrongTypedefLoggable(Ops)>>
  ValueBuilder operator[](utils::StrongTypedef<Tag, std::string, Ops> key);

  /// @brief Emplaces new member w/o a check whether the key already exists.
  /// @warning May create invalid JSON with duplicate key.
  /// @throw `TypeMismatchException` if not object or null value.
  void EmplaceNocheck(std::string_view key, ValueBuilder value);

  /// @brief Remove key from object. If key is missing nothing happens.
  /// @throw `TypeMismatchException` if value is not an object.
  void Remove(std::string_view key);

  iterator begin();
  iterator end();

  /// @brief Returns whether the array or object is empty.
  /// @throw `TypeMismatchException` if not an array or an object.
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

  /// @brief Returns true if *this holds a map (Type::kObject).
  bool IsObject() const noexcept;

  /// @brief Returns array size or object members count.
  /// @throw `TypeMismatchException` if not an array or an object.
  std::size_t GetSize() const;

  /// @brief Returns true if value holds a `key`.
  /// @throw `TypeMismatchException` if `*this` is not a map or null.
  bool HasMember(std::string_view key) const;

  /// @brief Returns full path to this value.
  std::string GetPath() const;

  /// @brief Resize the array value or convert null value
  /// into an array of requested size.
  /// @throw `TypeMismatchException` if not an array or null.
  void Resize(std::size_t size);

  /// @brief Add element into the last position of array.
  /// @throw `TypeMismatchException` if not an array or null.
  void PushBack(ValueBuilder&& bld);

  /// @brief Take out the resulting `Value` object.
  /// After calling this method the object is in unspecified
  /// (but valid - possibly null) state.
  /// @throw `JsonException` if called not from the root builder.
  formats::json::Value ExtractValue();

 private:
  class EmplaceEnabler {};

 public:
  /// @cond
  ValueBuilder(EmplaceEnabler, impl::MutableValueWrapper) noexcept;
  /// @endcond

 private:
  enum class CheckMemberExists { kYes, kNo };

  explicit ValueBuilder(impl::MutableValueWrapper) noexcept;

  static void Copy(impl::Value& to, const ValueBuilder& from);
  static void Move(impl::Value& to, ValueBuilder&& from);

  impl::Value& AddMember(std::string_view key, CheckMemberExists);

  template <typename T>
  static Value DoSerialize(const T& t);

  impl::MutableValueWrapper value_;

  friend class Iterator<IterTraits, common::IteratorDirection::kForward>;
  friend class Iterator<IterTraits, common::IteratorDirection::kReverse>;
};

template <typename T>
Value ValueBuilder::DoSerialize(const T& t) {
  static_assert(
      formats::common::impl::kHasSerialize<Value, T>,
      "There is no `Serialize(const T&, formats::serialize::To<json::Value>)` "
      "in namespace of `T` or `formats::serizalize`. "
      ""
      "Probably you forgot to include the "
      "<userver/formats/serialize/common_containers.hpp> header "
      "or one of the <formats/json/serialize_*.hpp> headers or you "
      "have not provided a `Serialize` function overload.");

  return Serialize(t, formats::serialize::To<Value>());
}

template <typename T>
std::enable_if_t<std::is_integral<T>::value && sizeof(T) <= sizeof(int64_t),
                 Value>
Serialize(T value, formats::serialize::To<Value>) {
  using Type = std::conditional_t<std::is_signed<T>::value, int64_t, uint64_t>;
  return json::ValueBuilder(static_cast<Type>(value)).ExtractValue();
}

json::Value Serialize(std::chrono::system_clock::time_point tp,
                      formats::serialize::To<Value>);

template <typename Tag, utils::StrongTypedefOps Ops, typename Enable>
ValueBuilder ValueBuilder::operator[](
    utils::StrongTypedef<Tag, std::string, Ops> key) {
  return (*this)[std::move(key.GetUnderlying())];
}

/// Optimized maps of StrongTypedefs serialization for JSON
template <typename T>
std::enable_if_t<meta::kIsUniqueMap<T> &&
                     utils::IsStrongTypedefLoggable(T::key_type::kOps),
                 Value>
Serialize(const T& value, formats::serialize::To<Value>) {
  json::ValueBuilder builder(formats::common::Type::kObject);
  for (const auto& [key, value] : value) {
    builder.EmplaceNocheck(key.GetUnderlying(), value);
  }
  return builder.ExtractValue();
}

/// Optimized maps serialization for JSON
template <typename T>
std::enable_if_t<meta::kIsUniqueMap<T> &&
                     std::is_convertible_v<typename T::key_type, std::string>,
                 Value>
Serialize(const T& value, formats::serialize::To<Value>) {
  json::ValueBuilder builder(formats::common::Type::kObject);
  for (const auto& [key, value] : value) {
    builder.EmplaceNocheck(key, value);
  }
  return builder.ExtractValue();
}

}  // namespace formats::json

USERVER_NAMESPACE_END
