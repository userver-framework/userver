#pragma once

#include <formats/json/value.hpp>

namespace formats {
namespace json {

class ValueBuilder {
 public:
  struct IterTraits {
    using native_iter = Json::Value::iterator;
    using value_type = formats::json::ValueBuilder;
    using reference = formats::json::ValueBuilder&;
    using pointer = formats::json::ValueBuilder*;
  };

  using iterator = Iterator<IterTraits>;

 public:
  ValueBuilder() noexcept = default;
  ValueBuilder(Json::Value&& val);
  ValueBuilder(Type type);

  ValueBuilder(const ValueBuilder& other);
  ValueBuilder(ValueBuilder&& other);
  ValueBuilder& operator=(const ValueBuilder& other);
  ValueBuilder& operator=(ValueBuilder&& other);

  ValueBuilder(const formats::json::Value& other);
  ValueBuilder(formats::json::Value&& other);

  /// Converting constructor.
  template <typename T>
  ValueBuilder(
      T t, typename std::enable_if<std::is_arithmetic<T>::value ||
                                   std::is_same<T, std::string>::value>::type* =
               nullptr)
      : value_(std::make_shared<Json::Value>(std::move(t))) {}
  ValueBuilder(const char* str);
  ValueBuilder(size_t t);

  /// @brief Access member by key for modification.
  /// @throw `TypeMismatchException` if not object or null value.
  ValueBuilder operator[](const std::string& key);
  /// @brief Access array member by index for modification.
  /// @throw `TypeMismatchException` if not array value.
  /// @throw `OutOfBoundsException` if index is greater than size.
  ValueBuilder operator[](uint32_t index);

  iterator begin();
  iterator end();

  /// @brief Resize the array value or convert null value
  /// into an array of requested size.
  /// @throw `TypeMismatchException` if not array or null value.
  void Resize(uint32_t size);

  /// @brief Add element into the last position of array.
  /// @throw `TypeMismatchException` if not array or null value.
  void PushBack(ValueBuilder&& bld);

  /// @brief Take out the resulting `Value` object.
  /// After calling this method the object is in unspecified
  /// (but valid - possibly null) state.
  /// @throw `JsonException` if called not from the root builder.
  formats::json::Value ExtractValue();

 private:
  ValueBuilder(const NativeValuePtr& root, const Json::Value& val,
               const formats::json::Path& path, const std::string& key);
  ValueBuilder(const NativeValuePtr& root, const Json::Value& val,
               const formats::json::Path& path, uint32_t index);

  // For iterator interface compatibility
  void Set(const NativeValuePtr& root, const Json::Value& val,
           const formats::json::Path& path, const std::string& key);
  void Set(const NativeValuePtr& root, const Json::Value& val,
           const formats::json::Path& path, uint32_t index);
  const Json::Value& Get() const;
  std::string GetPath() const;

  void Copy(const ValueBuilder& other);

 private:
  formats::json::Value value_;

  friend class Iterator<IterTraits>;
};

}  // namespace json
}  // namespace formats
