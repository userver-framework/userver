#pragma once

#include <type_traits>

#include <json/value.h>

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

  bool isMissing() const;
  bool isNull() const;
  bool isBool() const;
  bool isInt() const;
  bool isInt64() const;
  bool isUInt() const;
  bool isUInt64() const;
  bool isIntegral() const;
  bool isFloat() const;
  bool isDouble() const;
  bool isNumeric() const;
  bool isString() const;
  bool isArray() const;
  bool isObject() const;

  bool asBool() const;
  int32_t asInt() const;
  int64_t asInt64() const;
  uint32_t asUInt() const;
  uint64_t asUInt64() const;
  float asFloat() const;
  double asDouble() const;
  std::string asString() const;

  bool HasMember(const char* key) const;
  bool HasMember(const std::string& key) const;

  std::string GetPath() const;
  Value Clone() const;

 protected:
  Value(NativeValuePtr&& root) noexcept;
  const Json::Value& Get() const;
  bool IsRoot() const;

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

  void CheckArrayOrNull() const;
  void CheckObjectOrNull() const;
  void CheckObjectOrArray() const;
  void CheckOutOfBounds(uint32_t index) const;

 private:
  NativeValuePtr root_;
  Json::Value* value_ptr_;
  formats::json::Path path_;

  friend class Iterator<IterTraits>;
  friend class ValueBuilder;
};

}  // namespace json
}  // namespace formats
