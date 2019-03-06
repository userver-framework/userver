#pragma once

#include <limits>
#include <type_traits>

#include <json/value.h>

#include <formats/json/exception.hpp>
#include <formats/json/iterator.hpp>
#include <formats/json/types.hpp>

namespace formats {
namespace json {

class ValueBuilder;

class Value {
 public:
  struct IterTraits {
    using native_iter = Json::Value::const_iterator;
    using value_type = formats::json::Value;
    using reference = formats::json::Value&;
    using pointer = formats::json::Value*;
  };

  using const_iterator = Iterator<IterTraits>;

 public:
  Value() noexcept;

  /// @brief Access member by key for read.
  /// @throw `TypeMismatchException` if not object or null value.
  Value operator[](const std::string& key) const;
  /// @brief Access array member by index for read.
  /// @throw `TypeMismatchException` if not array value.
  /// @throw `OutOfBoundsException` if index is greater or equal
  /// than size.
  Value operator[](uint32_t index) const;

  const_iterator begin() const;
  const_iterator end() const;

  /// @brief Returns array size or object members count.
  /// @throw `TypeMismatchException` if not array or object value.
  uint32_t GetSize() const;

  bool operator==(const Value& other) const;
  bool operator!=(const Value& other) const;

  bool IsMissing() const;

  bool IsNull() const;
  bool IsBool() const;
  bool IsInt() const;
  bool IsInt64() const;
  bool IsUInt64() const;
  bool IsDouble() const;
  bool IsString() const;
  bool IsArray() const;
  bool IsObject() const;

  template <typename T>
  T As() const;

  template <typename T, typename First, typename... Rest>
  T As(First&& default_arg, Rest&&... more_default_args) const;

  bool HasMember(const char* key) const;
  bool HasMember(const std::string& key) const;

  std::string GetPath() const;
  Value Clone() const;

  void CheckNotMissing() const;
  void CheckArrayOrNull() const;
  void CheckObjectOrNull() const;
  void CheckObjectOrArrayOrNull() const;
  void CheckInBounds(uint32_t index) const;

 protected:
  Value(NativeValuePtr&& root) noexcept;
  const Json::Value& Get() const;
  bool IsRoot() const;
  bool IsUniqueReference() const;

 private:
  Value(const NativeValuePtr& root, const Json::Value* value_ptr,
        const formats::json::Path& path, const std::string& key);
  Value(const NativeValuePtr& root, const Json::Value& val,
        const formats::json::Path& path, uint32_t index);

  void Set(const NativeValuePtr& root, const Json::Value& val,
           const formats::json::Path& path, const std::string& key);
  void Set(const NativeValuePtr& root, const Json::Value& val,
           const formats::json::Path& path, uint32_t index);

  void EnsureValid();
  const Json::Value& GetNative() const;
  Json::Value& GetNative();

 private:
  NativeValuePtr root_;
  Json::Value* value_ptr_;
  formats::json::Path path_;

  friend class Iterator<IterTraits>;
  friend class ValueBuilder;
};

template <typename T>
T Value::As() const {
  const T* dont_use_me = nullptr;
  return ParseJson(*this, dont_use_me);
}

template <>
bool Value::As<bool>() const;

template <>
int32_t Value::As<int32_t>() const;

template <>
int64_t Value::As<int64_t>() const;

template <>
uint64_t Value::As<uint64_t>() const;

#ifdef _LIBCPP_VERSION
template <>
inline unsigned long Value::As<unsigned long>() const {
  return As<uint64_t>();
}
#endif

template <>
double Value::As<double>() const;

template <>
std::string Value::As<std::string>() const;

template <typename T, typename First, typename... Rest>
T Value::As(First&& default_arg, Rest&&... more_default_args) const {
  if (IsMissing()) {
    return T(std::forward<First>(default_arg),
             std::forward<Rest>(more_default_args)...);
  }
  return As<T>();
}

template <typename T>
std::enable_if_t<std::is_integral<T>::value && (sizeof(T) > 1), T> ParseJson(
    const formats::json::Value& value, const T*) {
  using IntT = std::conditional_t<std::is_signed<T>::value, int64_t, uint64_t>;

  auto val = value.As<IntT>();
  auto min = static_cast<IntT>(std::numeric_limits<T>::min());
  auto max = static_cast<IntT>(std::numeric_limits<T>::max());
  if (val < min || val > max)
    throw IntegralOverflowException(min, val, max, value.GetPath());

  return val;
}

}  // namespace json
}  // namespace formats
