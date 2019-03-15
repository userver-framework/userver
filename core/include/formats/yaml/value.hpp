#pragma once

#include <limits>
#include <type_traits>

#include <formats/yaml/exception.hpp>
#include <formats/yaml/iterator.hpp>
#include <formats/yaml/types.hpp>

#include <utils/fast_pimpl.hpp>

namespace formats {
namespace yaml {

class ValueBuilder;

class Value {
 public:
  struct IterTraits {
    using native_iter = YAML::const_iterator;
    using value_type = formats::yaml::Value;
    using reference = formats::yaml::Value&;
    using pointer = formats::yaml::Value*;
  };

  using const_iterator = Iterator<IterTraits>;

 public:
  Value() noexcept;

  Value(Value&&);
  Value(const Value&);
  Value& operator=(Value&&);
  Value& operator=(const Value&);

  ~Value();

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

  bool HasMember(const char* key) const;
  bool HasMember(const std::string& key) const;

  std::string GetPath() const;
  Value Clone() const;

  void CheckNotMissing() const;
  void CheckArrayOrNull() const;
  void CheckObjectOrNull() const;
  void CheckObjectOrArrayOrNull() const;
  void CheckInBounds(uint32_t index) const;

  bool IsRoot() const;
  bool DebugIsReferencingSameMemory(const Value& other) const;

 private:
  Value(const YAML::Node& root) noexcept;

  static Value MakeNonRoot(const YAML::Node& value,
                           const formats::yaml::Path& path,
                           const std::string& key);
  static Value MakeNonRoot(const YAML::Node& val,
                           const formats::yaml::Path& path, uint32_t index);

  void SetNonRoot(const YAML::Node& val, const formats::yaml::Path& path,
                  const std::string& key);

  void SetNonRoot(const YAML::Node& val, const formats::yaml::Path& path,
                  uint32_t index);

  void EnsureValid() const;
  const YAML::Node& GetNative() const;
  YAML::Node& GetNative();

 private:
  template <class T>
  bool IsConvertible() const;

  template <class T>
  T ValueAs() const;

  bool is_root_;

  static constexpr std::size_t kNativeNodeSize = 32;
  static constexpr std::size_t kNativeAlignment = alignof(void*);
  utils::FastPimpl<YAML::Node, kNativeNodeSize, kNativeAlignment> value_pimpl_;
  formats::yaml::Path path_;

  friend class Iterator<IterTraits>;
  friend class ValueBuilder;

  friend formats::yaml::Value FromString(const std::string&);
  friend formats::yaml::Value FromStream(std::istream&);
  friend void Serialize(const formats::yaml::Value&, std::ostream&);
};

template <typename T>
T Value::As() const {
  const T* dont_use_me = nullptr;
  return ParseYaml(*this, dont_use_me);
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

template <typename T>
std::enable_if_t<std::is_integral<T>::value && (sizeof(T) > 1), T> ParseYaml(
    const formats::yaml::Value& value, const T*) {
  using IntT = std::conditional_t<std::is_signed<T>::value, int64_t, uint64_t>;

  auto val = value.As<IntT>();
  auto min = static_cast<IntT>(std::numeric_limits<T>::min());
  auto max = static_cast<IntT>(std::numeric_limits<T>::max());
  if (val < min || val > max)
    throw IntegralOverflowException(min, val, max, value.GetPath());

  return val;
}

}  // namespace yaml
}  // namespace formats
