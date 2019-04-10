#pragma once

#include <formats/yaml/value.hpp>

namespace formats {
namespace yaml {

class ValueBuilder {
 public:
  struct IterTraits {
    using native_iter = YAML::iterator;
    using value_type = formats::yaml::ValueBuilder;
    using reference = formats::yaml::ValueBuilder&;
    using pointer = formats::yaml::ValueBuilder*;
  };

  using iterator = Iterator<IterTraits>;

 public:
  /// Constructs a valueBuilder that holds kNull
  ValueBuilder();

  /// Constructs a valueBuilder that holds default value for provided `type`.
  ValueBuilder(Type type);

  ValueBuilder(const ValueBuilder& other);
  // NOLINTNEXTLINE(performance-noexcept-move-constructor,bugprone-exception-escape)
  ValueBuilder(ValueBuilder&& other);
  // NOLINTNEXTLINE(bugprone-exception-escape)
  ValueBuilder& operator=(const ValueBuilder& other);
  // NOLINTNEXTLINE(performance-noexcept-move-constructor,bugprone-exception-escape)
  ValueBuilder& operator=(ValueBuilder&& other);

  ValueBuilder(const formats::yaml::Value& other);
  ValueBuilder(formats::yaml::Value&& other);

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
  /// @throw `YamlException` if called not from the root builder.
  formats::yaml::Value ExtractValue();

 private:
  static ValueBuilder MakeNonRoot(const YAML::Node& val,
                                  const formats::yaml::Path& path,
                                  const std::string& key);
  static ValueBuilder MakeNonRoot(const YAML::Node& val,
                                  const formats::yaml::Path& path,
                                  std::size_t index);

  // For iterator interface compatibility
  void SetNonRoot(const YAML::Node& val, const formats::yaml::Path& path,
                  const std::string& key);
  void SetNonRoot(const YAML::Node& val, const formats::yaml::Path& path,
                  std::size_t index);
  std::string GetPath() const;

  void Copy(const ValueBuilder& from);
  void Move(ValueBuilder&& from);
  void NodeDataAssign(const formats::yaml::Value& from);

 private:
  formats::yaml::Value value_;

  friend class Iterator<IterTraits>;
};

}  // namespace yaml
}  // namespace formats
