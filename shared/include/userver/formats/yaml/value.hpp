#pragma once

/// @file userver/formats/yaml/value.hpp
/// @brief @copybrief formats::yaml::Value

#include <type_traits>

#include <userver/formats/common/items.hpp>
#include <userver/formats/common/meta.hpp>
#include <userver/formats/parse/common.hpp>
#include <userver/formats/yaml/exception.hpp>
#include <userver/formats/yaml/iterator.hpp>
#include <userver/formats/yaml/types.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::yaml {

class ValueBuilder;

/// @ingroup userver_containers userver_formats
///
/// @brief Non-mutable YAML value representation.
///
/// Class provides non mutable access YAML value. For modification and
/// construction of new YAML values use formats::yaml::ValueBuilder.
///
/// ## Example usage:
///
/// @snippet formats/yaml/value_test.cpp  Sample formats::yaml::Value usage
///
/// @see @ref md_en_userver_formats
class Value final {
 public:
  struct IterTraits {
    using native_iter = YAML::const_iterator;
    using value_type = formats::yaml::Value;
    using reference = const formats::yaml::Value&;
    using pointer = const formats::yaml::Value*;
  };
  struct DefaultConstructed {};

  using const_iterator = Iterator<IterTraits>;
  using Exception = formats::yaml::Exception;
  using ParseException = formats::yaml::ParseException;
  using Builder = ValueBuilder;

  /// @brief Constructs a Value that holds a Null.
  Value() noexcept;

  // NOLINTNEXTLINE(performance-noexcept-move-constructor)
  Value(Value&&);
  Value(const Value&);
  // NOLINTNEXTLINE(performance-noexcept-move-constructor)
  Value& operator=(Value&&);
  Value& operator=(const Value&);

  template <class T>
  Value& operator=(T&&) && {
    static_assert(!sizeof(T),
                  "You're assigning to a temporary formats::yaml::Value! Use "
                  "formats::yaml::ValueBuilder for data modifications.");
    return *this;
  }

  ~Value();

  /// @brief Copies `other`, appending `path_prefix` to the stored `path`
  /// @warning `other` must have an empty (root) path
  /// @throw `std::logic_error` if `other` has path other than the root path
  /// @see GetPath
  Value(Value&& other, std::string path_prefix);

  /// @brief Access member by key for read.
  /// @throw TypeMismatchException if not a missing value, an object, or Null.
  Value operator[](std::string_view key) const;

  /// @brief Access array member by index for read.
  /// @throw TypeMismatchException if not an array value.
  /// @throw `OutOfBoundsException` if index is greater or equal
  /// than size.
  Value operator[](std::size_t index) const;

  /// @brief Returns an iterator to the beginning of the held array or map.
  /// @throw TypeMismatchException if not an array or an object.
  const_iterator begin() const;

  /// @brief Returns an iterator to the end of the held array or map.
  /// @throw TypeMismatchException if not an array or an object.
  const_iterator end() const;

  /// @brief Returns whether the array or object is empty.
  /// @throw TypeMismatchException if not an array or an object.
  bool IsEmpty() const;

  /// @brief Returns array size or object members count.
  /// @throw TypeMismatchException if not an array or an object.
  std::size_t GetSize() const;

  /// @brief Compares values.
  /// @throw MemberMissingException if `*this` or `other` is missing.
  bool operator==(const Value& other) const;
  bool operator!=(const Value& other) const;

  /// @brief Returns true if *this holds nothing. When `IsMissing()` returns
  /// `true` any attempt to get the actual value or iterate over *this will
  /// throw MemberMissingException.
  bool IsMissing() const;

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

  /// @brief Returns true if *this is a map (Type::kObject).
  bool IsObject() const noexcept;

  // clang-format off

  /// @brief Returns value of *this converted to T.
  /// @throw Anything derived from std::exception.
  ///
  /// ## Example usage:
  ///
  /// @snippet formats/yaml/value_test.cpp  Sample formats::yaml::Value::As<T>() usage
  ///
  /// @see @ref md_en_userver_formats

  // clang-format on

  template <typename T>
  T As() const;

  /// @brief Returns value of *this converted to T or T(args) if
  /// this->IsMissing().
  /// @throw Anything derived from std::exception.
  template <typename T, typename First, typename... Rest>
  T As(First&& default_arg, Rest&&... more_default_args) const;

  /// @brief Returns value of *this converted to T or T() if this->IsMissing().
  /// @throw Anything derived from std::exception.
  /// @note Use as `value.As<T>({})`
  template <typename T>
  T As(DefaultConstructed) const;

  /// @brief Returns true if *this holds a `key`.
  bool HasMember(std::string_view key) const;

  /// @brief Returns full path to this value.
  std::string GetPath() const;

  /// @brief Returns 0-based column number of this `Value` in the original
  /// document. Returns `-1` if `this->IsMissing()`. If `Value` was created
  /// using formats::yaml::ValueBuilder, returns `0`.
  /// @note This method available **only** for formats::yaml::Value.
  int GetColumn() const;

  /// @brief Returns 0-based line number of this `Value` in the original
  /// document. Returns `-1` if `this->IsMissing()`. If `Value` was created
  /// using formats::yaml::ValueBuilder, returns `0`.
  /// @note This method available **only** for formats::yaml::Value.
  int GetLine() const;

  /// @brief Returns new value that is an exact copy if the existing one
  /// but references different memory (a deep copy of a *this). The returned
  /// value is a root value with path '/'.
  /// @throws MemberMissingException id `this->IsMissing()`.
  Value Clone() const;

  /// @throw MemberMissingException if `this->IsMissing()`.
  void CheckNotMissing() const;

  /// @throw MemberMissingException if `*this` is not an array.
  void CheckArray() const;

  /// @throw MemberMissingException if `*this` is not an array or Null.
  void CheckArrayOrNull() const;

  /// @throw TypeMismatchException if `*this` is not a map or Null.
  void CheckObjectOrNull() const;

  /// @throw TypeMismatchException if `*this` is not a map.
  void CheckObject() const;

  /// @throw TypeMismatchException if `*this` is not convertible to std::string.
  void CheckString() const;

  /// @throw TypeMismatchException if `*this` is not a map, array or Null.
  void CheckObjectOrArrayOrNull() const;

  /// @throw TypeMismatchException if `*this` is not a map, array or Null;
  /// `OutOfBoundsException` if `index >= this->GetSize()`.
  void CheckInBounds(std::size_t index) const;

  /// @brief Returns true if *this is a first (root) value.
  bool IsRoot() const noexcept;

  /// @brief Returns true if `*this` and `other` reference the value by the same
  /// pointer.
  bool DebugIsReferencingSameMemory(const Value& other) const;

 private:
  class EmplaceEnabler {};

 public:
  /// @cond
  Value(EmplaceEnabler, const YAML::Node& value,
        const formats::yaml::Path& path, std::string_view key);

  Value(EmplaceEnabler, const YAML::Node& value,
        const formats::yaml::Path& path, size_t index);
  /// @endcond

 private:
  Value(const YAML::Node& root) noexcept;

  static Value MakeNonRoot(const YAML::Node& value,
                           const formats::yaml::Path& path,
                           std::string_view key);
  static Value MakeNonRoot(const YAML::Node& val,
                           const formats::yaml::Path& path, size_t index);

  const YAML::Node& GetNative() const;
  YAML::Node& GetNative();
  int GetExtendedType() const;

  template <class T>
  bool IsConvertible() const;

  template <class T>
  T ValueAs() const;

  static constexpr std::size_t kNativeNodeSize = 64;
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
  static_assert(formats::common::impl::kHasParse<Value, T>,
                "There is no `Parse(const Value&, formats::parse::To<T>)` in "
                "namespace of `T` or `formats::parse`. "
                "Probably you forgot to include the "
                "<userver/formats/parse/common_containers.hpp> or you "
                "have not provided a `Parse` function overload.");

  return Parse(*this, formats::parse::To<T>{});
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

template <typename T, typename First, typename... Rest>
T Value::As(First&& default_arg, Rest&&... more_default_args) const {
  if (IsMissing() || IsNull()) {
    // intended raw ctor call, sometimes casts
    // NOLINTNEXTLINE(google-readability-casting)
    return T(std::forward<First>(default_arg),
             std::forward<Rest>(more_default_args)...);
  }
  return As<T>();
}

template <typename T>
T Value::As(Value::DefaultConstructed) const {
  return (IsMissing() || IsNull()) ? T() : As<T>();
}

/// @brief Wrapper for handy python-like iteration over a map
///
/// @code
///   for (const auto& [name, value]: Items(map)) ...
/// @endcode
using formats::common::Items;

}  // namespace formats::yaml

/// Although we provide user defined literals, please beware that
/// 'using namespace ABC' may contradict code style of your company.
namespace formats::literals {

yaml::Value operator"" _yaml(const char* str, std::size_t len);

}  // namespace formats::literals

USERVER_NAMESPACE_END
