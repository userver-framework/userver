#pragma once

#include <formats/json/value.hpp>

namespace formats {
namespace json {

class ValueBuilder {
 public:
  struct IterTraits {
    using native_iter = Json::ValueIterator;
    using value_type = formats::json::ValueBuilder;
    using reference = formats::json::ValueBuilder&;
    using pointer = formats::json::ValueBuilder*;
  };

  using iterator = Iterator<IterTraits>;

 public:
  /// Constructs a valueBuilder that holds kNull
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
#ifdef _LIBCPP_VERSION  // In libc++ long long and int64_t are the same
  ValueBuilder(long t);
  ValueBuilder(unsigned long t);
#else
  ValueBuilder(long long t);
  ValueBuilder(unsigned long long t);
#endif
  ValueBuilder(float t);
  ValueBuilder(double t);

  /// @brief Access member by key for modification.
  /// @throw `TypeMismatchException` if not object or null value.
  ValueBuilder operator[](const std::string& key);
  /// @brief Access array member by index for modification.
  /// @throw `TypeMismatchException` if not array value.
  /// @throw `OutOfBoundsException` if index is greater than size.
  ValueBuilder operator[](std::size_t index);

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
  ValueBuilder(const NativeValuePtr& root, const Json::Value& val,
               const formats::json::Path& path, const std::string& key);
  ValueBuilder(const NativeValuePtr& root, const Json::Value& val,
               const formats::json::Path& path, std::size_t index);

  // For iterator interface compatibility
  void SetNonRoot(const NativeValuePtr& root, const Json::Value& val,
                  const formats::json::Path& path, const std::string& key);
  void SetNonRoot(const NativeValuePtr& root, const Json::Value& val,
                  const formats::json::Path& path, std::size_t index);
  std::string GetPath() const;

  void Copy(Json::Value& to, const ValueBuilder& from);
  void Move(Json::Value& to, ValueBuilder&& from);

 private:
  formats::json::Value value_;

  friend class Iterator<IterTraits>;
};

}  // namespace json
}  // namespace formats
