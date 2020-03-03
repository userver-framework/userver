#pragma once

/// @file formats/json/value.hpp
/// @brief @copybrief formats::json::Value

#include <type_traits>

#include <formats/common/meta.hpp>
#include <formats/json/exception.hpp>
#include <formats/json/iterator.hpp>
#include <formats/json/types.hpp>
#include <formats/parse/common.hpp>

#include <utils/string_view.hpp>

namespace formats::json {

class ValueBuilder;

/// @brief Non-mutable JSON value representation.
///
/// Class provides non mutable access JSON value. For modification and
/// construction of new JSON values use formats::json::ValueBuilder.
class Value final {
 public:
  struct IterTraits {
    using value_type = formats::json::Value;
    using reference = formats::json::Value&;
    using pointer = formats::json::Value*;
  };

  using const_iterator = Iterator<IterTraits>;
  using Exception = formats::json::Exception;
  using ParseException = formats::json::ParseException;
  using Builder = ValueBuilder;

 public:
  /// @brief Constructs a Value that holds a Null.
  Value() noexcept;

  Value(const Value&) = default;
  Value(Value&&) noexcept = default;

  Value& operator=(const Value&) & = default;
  Value& operator=(Value&& other) & noexcept;

  template <class T>
  Value& operator=(T&&) && {
    static_assert(false && sizeof(T),
                  "You're assigning to a temporary formats::json::Value! Use "
                  "formats::json::ValueBuilder for data modifications.");
    return *this;
  }

  ~Value();

  /// @brief Access member by key for read.
  /// @throw TypeMismatchException if not object or null value.
  Value operator[](const std::string& key) const;
  /// @brief Access array member by index for read.
  /// @throw TypeMismatchException if not array value.
  /// @throw OutOfBoundsException if index is greater or equal
  /// than size.
  Value operator[](std::size_t index) const;

  /// @brief Returns an iterator to the beginning of the held array or map.
  /// @throw TypeMismatchException is the value of *this is not a map, array
  /// or Null.
  const_iterator begin() const;

  /// @brief Returns an iterator to the end of the held array or map.
  /// @throw TypeMismatchException is the value of *this is not a map, array
  /// or Null.
  const_iterator end() const;

  /// @brief Returns whether the array or object is empty.
  /// @throw TypeMismatchException if not array or object value.
  bool IsEmpty() const;

  /// @brief Returns array size or object members count.
  /// @throw TypeMismatchException if not array or object value.
  std::size_t GetSize() const;

  /// @brief Compares values.
  /// @throw MemberMissingException if `*this` or `other` is missing.
  bool operator==(const Value& other) const;
  bool operator!=(const Value& other) const;

  /// @brief Returns true if *this holds nothing. When `IsMissing()` returns
  /// `true` any attempt to get the actual value or iterate over *this will
  /// @throw Nothing.
  bool IsMissing() const noexcept;

  /// @brief Returns true if *this holds a Null (Type::kNull).
  /// @throw Nothing.
  bool IsNull() const;

  /// @brief Returns true if *this holds a bool.
  /// @throw Nothing.
  bool IsBool() const;

  /// @brief Returns true if *this holds an int.
  /// @throw Nothing.
  bool IsInt() const;

  /// @brief Returns true if *this holds an int64_t.
  /// @throw Nothing.
  bool IsInt64() const;

  /// @brief Returns true if *this holds an uint64_t.
  /// @throw Nothing.
  bool IsUInt64() const;

  /// @brief Returns true if *this holds a double.
  /// @throw Nothing.
  bool IsDouble() const;

  /// @brief Returns true if *this is holds a std::string.
  /// @throw Nothing.
  bool IsString() const;

  /// @brief Returns true if *this is holds an array (Type::kArray).
  /// @throw Nothing.
  bool IsArray() const;

  /// @brief Returns true if *this holds a map (Type::kObject).
  /// @throw Nothing.
  bool IsObject() const;

  /// @brief Returns value of *this converted to T.
  /// @throw Anything derived from std::exception.
  template <typename T>
  T As() const;

  /// @brief Returns value of *this converted to T or T(args) if
  /// this->IsMissing().
  /// @throw Anything derived from std::exception.
  template <typename T, typename First, typename... Rest>
  T As(First&& default_arg, Rest&&... more_default_args) const;

  /// @brief Extracts the specified type with relaxed type checks.
  /// For example, `true` may be converted to 1.0.
  template <typename T>
  T ConvertTo() const;

  /// Extracts the specified type with strict type checks, or constructs the
  /// default value when the field is not present
  template <typename T, typename First, typename... Rest>
  T ConvertTo(First&& default_arg, Rest&&... more_default_args) const;

  /// @brief Returns true if *this holds a `key`.
  /// @throw Nothing.
  bool HasMember(const char* key) const;

  /// @brief Returns true if *this holds a `key`.
  /// @throw Nothing.
  bool HasMember(const std::string& key) const;

  /// @brief Returns full path to this value.
  std::string GetPath() const;

  /// @brief Returns new value that is an exact copy if the existing one
  /// but references different memory (a deep copy of a *this). The returned
  /// value is a root value with path '/'.
  /// @throws MemberMissingException id `this->IsMissing()`.
  Value Clone() const;

  /// @throw MemberMissingException if `this->IsMissing()`.
  void CheckNotMissing() const;

  /// @throw MemberMissingException if `*this` is not an array or Null.
  void CheckArrayOrNull() const;

  /// @throw TypeMismatchException if `*this` is not a map or Null.
  void CheckObjectOrNull() const;

  /// @throw TypeMismatchException if `*this` is not a map.
  void CheckObject() const;

  /// @throw TypeMismatchException if `*this` is not a map, array or Null.
  void CheckObjectOrArrayOrNull() const;

  /// @throw TypeMismatchException if `*this` is not a map, array or Null;
  /// `OutOfBoundsException` if `index >= this->GetSize()`.
  void CheckInBounds(std::size_t index) const;

  /// @brief Returns true if *this is a first (root) value.
  bool IsRoot() const noexcept;

  /// @brief Returns true if `*this` and `other` reference the value by the same
  /// pointer.
  bool DebugIsReferencingSameMemory(const Value& other) const {
    return value_ptr_ == other.value_ptr_;
  }

 private:
  Value(NativeValuePtr&& root) noexcept;
  bool IsUniqueReference() const;

  Value(const NativeValuePtr& root, const impl::Value* value_ptr, int depth);
  Value(const NativeValuePtr& root, std::string&& detached_path);

  void SetNonRoot(const NativeValuePtr& root, const impl::Value& val,
                  int depth);
  void SetNonRoot(const NativeValuePtr& root, std::string&& detached_path);

  void EnsureNotMissing();
  const impl::Value& GetNative() const;
  impl::Value& GetNative();
  int GetExtendedType() const;

 private:
  NativeValuePtr root_;
  impl::Value* value_ptr_{nullptr};
  /// Full path of node (only for missing nodes)
  std::string detached_path_;
  /// Depth of the node to ease recursive traversal in GetPath()
  int depth_;

  friend class Iterator<IterTraits>;
  friend class ValueBuilder;

  friend formats::json::Value FromString(utils::string_view);
  friend formats::json::Value FromStream(std::istream&);
  friend void Serialize(const formats::json::Value&, std::ostream&);
  friend std::string ToString(const formats::json::Value&);
};

template <typename T>
T Value::As() const {
  if constexpr (kHasParseJsonFor<T>) {
    const T* dont_use_me = nullptr;
    return ParseJson(*this, dont_use_me);  // TODO: deprecate
  } else {
    static_assert(
        formats::common::kHasParseTo<Value, T>,
        "There is no `ParseJson(const formats::json::Value&, const T*)` "
        "in namespace of `T` and no "
        "`Parse(const Value&, formats::parse::To<T>)` in namespace of "
        "`T` or `formats::parse`."
        ""
        "Probably you forgot to include the "
        "<formats/parse/common_containers.hpp> or you "
        "have not provided a `ParseJson` or `Parse` function overload.");

    return Parse(*this, formats::parse::To<T>{});
  }
}

template <>
bool Value::As<bool>() const;

template <>
int64_t Value::As<int64_t>() const;

template <>
uint64_t Value::As<uint64_t>() const;

template <>
double Value::As<double>() const;

template <>
std::string Value::As<std::string>() const;

template <>
bool Value::ConvertTo<bool>() const;

template <>
int64_t Value::ConvertTo<int64_t>() const;

template <>
uint64_t Value::ConvertTo<uint64_t>() const;

template <>
double Value::ConvertTo<double>() const;

template <>
std::string Value::ConvertTo<std::string>() const;

template <typename T, typename First, typename... Rest>
T Value::As(First&& default_arg, Rest&&... more_default_args) const {
  if (IsMissing()) {
    return T(std::forward<First>(default_arg),
             std::forward<Rest>(more_default_args)...);
  }
  return As<T>();
}

template <typename T>
T Value::ConvertTo() const {
  static_assert(
      formats::common::kHasConvertTo<Value, T>,
      "There is no `Convert(const Value&, formats::parse::To<T>)` in "
      "namespace of `T` or `formats::parse`."
      ""
      "Probably you have not provided a `Convert` function overload.");

  return Convert(*this, formats::parse::To<T>{});
}

template <typename T, typename First, typename... Rest>
T Value::ConvertTo(First&& default_arg, Rest&&... more_default_args) const {
  if (IsMissing() || IsNull()) {
    return T(std::forward<First>(default_arg),
             std::forward<Rest>(more_default_args)...);
  }
  return ConvertTo<T>();
}

inline Value Parse(const Value& value, parse::To<Value>) { return value; }

}  // namespace formats::json
