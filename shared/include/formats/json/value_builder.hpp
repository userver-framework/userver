#pragma once

/// @file formats/json/value_builder.hpp
/// @brief @copybrief formats::json::ValueBuilder

#include <chrono>

#include <formats/common/meta.hpp>
#include <formats/json/value.hpp>

namespace formats::json {

/// @brief Builder for JSON.
///
/// Class provides methods for building JSON. For read only access to the
/// existing JSON values use formats::json::Value.
class ValueBuilder final {
 public:
  struct IterTraits {
    using value_type = formats::json::ValueBuilder;
    using reference = formats::json::ValueBuilder&;
    using pointer = formats::json::ValueBuilder*;
  };

  using iterator = Iterator<IterTraits>;

 public:
  /// Constructs a ValueBuilder that holds kNull
  ValueBuilder() noexcept = default;

  /// Constructs a valueBuilder that holds default value for provided `type`.
  ValueBuilder(Type type);

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
  ValueBuilder operator[](const std::string& key);
  /// @brief Access array member by index for modification.
  /// @throw `TypeMismatchException` if not array value.
  /// @throw `OutOfBoundsException` if index is greater than size.
  ValueBuilder operator[](std::size_t index);

  /// @brief Emplaces new member w/o a check whether the key already exists.
  /// @throw `TypeMismatchException` if not object or null value.
  void EmplaceNocheck(const std::string& key, ValueBuilder value);

  /// @brief Remove key from object. If key is missing nothing happens.
  /// @throw `TypeMismatchException` if value is not an object.
  void Remove(const std::string& key);

  iterator begin();
  iterator end();

  /// @brief Returns array size or object members count.
  /// @throw `TypeMismatchException` if not array or object value.
  std::size_t GetSize() const;

  /// @brief Resize the array value or convert null value
  /// into an array of requested size.
  /// @throw `TypeMismatchException` if not array or null value.
  void Resize(std::size_t size);

  /// @brief Add element into the last position of array.
  /// @throw `TypeMismatchException` if not array or null value.
  void PushBack(ValueBuilder&& bld);

  /// @brief Take out the resulting `Value` object.
  /// After calling this method the object is in unspecified
  /// (but valid - possibly null) state.
  /// @throw `JsonException` if called not from the root builder.
  formats::json::Value ExtractValue();

 private:
  ValueBuilder(const NativeValuePtr& root, const impl::Value& val, int depth);

  // For iterator interface compatibility
  void SetNonRoot(const NativeValuePtr& root, const impl::Value& val,
                  int depth);
  std::string GetPath() const;

  void Copy(impl::Value& to, const ValueBuilder& from);
  void Move(impl::Value& to, ValueBuilder&& from);

  template <typename T>
  static Value DoSerialize(const T& t);

 private:
  formats::json::Value value_;

  friend class Iterator<IterTraits>;
};

template <typename T>
Value ValueBuilder::DoSerialize(const T& t) {
  static_assert(
      formats::common::kHasSerializeTo<Value, T>,
      "There is no `Serialize(const T&, formats::serialize::To<json::Value>)` "
      "in namespace of `T` or `formats::serizalize`. "
      ""
      "Probably you forgot to include the "
      "<formats/serialize/common_containers.hpp> or you "
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

}  // namespace formats::json
