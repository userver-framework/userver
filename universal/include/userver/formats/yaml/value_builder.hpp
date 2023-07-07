#pragma once

/// @file userver/formats/yaml/value_builder.hpp
/// @brief @copybrief formats::yaml::ValueBuilder

#include <userver/formats/common/transfer_tag.hpp>
#include <userver/formats/serialize/to.hpp>
#include <userver/formats/yaml/value.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::yaml {

// clang-format off

/// @ingroup userver_containers userver_formats
///
/// @brief Builder for YAML.
///
/// Class provides methods for building YAML. For read only access to the
/// existing YAML values use formats::yaml::Value.
///
/// ## Example usage:
///
/// @snippet formats/yaml/value_builder_test.cpp  Sample formats::yaml::ValueBuilder usage
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
    using native_iter = YAML::iterator;
    using value_type = formats::yaml::ValueBuilder;
    using reference = formats::yaml::ValueBuilder&;
    using pointer = formats::yaml::ValueBuilder*;
  };

  using iterator = Iterator<IterTraits>;

  /// Constructs a valueBuilder that holds kNull
  ValueBuilder();

  /// Constructs a valueBuilder that holds default value for provided `type`.
  ValueBuilder(Type type);

  ValueBuilder(const ValueBuilder& other);
  // NOLINTNEXTLINE(performance-noexcept-move-constructor)
  ValueBuilder(ValueBuilder&& other);
  ValueBuilder& operator=(const ValueBuilder& other);
  // NOLINTNEXTLINE(performance-noexcept-move-constructor)
  ValueBuilder& operator=(ValueBuilder&& other);

  ValueBuilder(const formats::yaml::Value& other);
  ValueBuilder(formats::yaml::Value&& other);

  /// Converting constructors.
  ValueBuilder(bool t);
  ValueBuilder(const char* str);
  ValueBuilder(const std::string& str);
  ValueBuilder(int t);
  ValueBuilder(unsigned int t);
  ValueBuilder(long t);
  ValueBuilder(unsigned long t);
  ValueBuilder(long long t);
  ValueBuilder(unsigned long long t);
  ValueBuilder(float t);
  ValueBuilder(double t);

  /// @brief Transfers the `ValueBuilder` object
  /// @see formats::common::TransferTag for the transfer semantics
  ValueBuilder(common::TransferTag, ValueBuilder&&) noexcept;

  /// Universal constructor using Serialize
  template <typename T>
  ValueBuilder(const T& t) : ValueBuilder(DoSerialize(t)) {}

  /// @brief Access member by key for modification.
  /// @throw `TypeMismatchException` if not object or null value.
  ValueBuilder operator[](const std::string& key);
  /// @brief Access array member by index for modification.
  /// @throw `TypeMismatchException` if not an array value.
  /// @throw `OutOfBoundsException` if index is greater than size.
  ValueBuilder operator[](std::size_t index);
  /// @brief Access member by key for modification.
  /// @throw `TypeMismatchException` if not object or null value.
  template <
      typename Tag, utils::StrongTypedefOps Ops,
      typename Enable = std::enable_if_t<utils::IsStrongTypedefLoggable(Ops)>>
  ValueBuilder operator[](
      const utils::StrongTypedef<Tag, std::string, Ops>& key);

  /// @brief Remove key from object. If key is missing nothing happens.
  /// @throw `TypeMismatchException` if value is not an object.
  void Remove(const std::string& key);

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
  bool HasMember(const char* key) const;

  /// @brief Returns true if value holds a `key`.
  bool HasMember(const std::string& key) const;

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
  /// @throw `YamlException` if called not from the root builder.
  formats::yaml::Value ExtractValue();

 private:
  class EmplaceEnabler {};

 public:
  /// @cond
  ValueBuilder(EmplaceEnabler, const YAML::Node& value,
               const formats::yaml::Path& path, const std::string& key);

  ValueBuilder(EmplaceEnabler, const YAML::Node& value,
               const formats::yaml::Path& path, size_t index);
  /// @endcond

 private:
  static ValueBuilder MakeNonRoot(const YAML::Node& val,
                                  const formats::yaml::Path& path,
                                  const std::string& key);
  static ValueBuilder MakeNonRoot(const YAML::Node& val,
                                  const formats::yaml::Path& path,
                                  size_t index);

  void Copy(const ValueBuilder& from);
  void Move(ValueBuilder&& from);
  void NodeDataAssign(const formats::yaml::Value& from);

  template <typename T>
  static Value DoSerialize(const T& t);

  formats::yaml::Value value_;

  friend class Iterator<IterTraits>;
};

template <typename Tag, utils::StrongTypedefOps Ops, typename Enable>
ValueBuilder ValueBuilder::operator[](
    const utils::StrongTypedef<Tag, std::string, Ops>& key) {
  return (*this)[key.GetUnderlying()];
}

template <typename T>
Value ValueBuilder::DoSerialize(const T& t) {
  static_assert(
      formats::common::impl::kHasSerialize<Value, T>,
      "There is no `Serialize(const T&, formats::serialize::To<yaml::Value>)` "
      "in namespace of `T` or `formats::serizalize`. "
      ""
      "Probably you forgot to include the "
      "<userver/formats/serialize/common_containers.hpp> or you "
      "have not provided a `Serialize` function overload.");

  return Serialize(t, formats::serialize::To<Value>());
}

template <typename T>
std::enable_if_t<std::is_integral<T>::value && sizeof(T) <= sizeof(long long),
                 Value>
Serialize(T value, formats::serialize::To<Value>) {
  using Type = std::conditional_t<std::is_signed<T>::value, long long,
                                  unsigned long long>;
  return yaml::ValueBuilder(static_cast<Type>(value)).ExtractValue();
}

}  // namespace formats::yaml

USERVER_NAMESPACE_END
