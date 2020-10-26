#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <formats/parse/common.hpp>
#include <formats/parse/common_containers.hpp>
#include <formats/yaml/value.hpp>

#include <yaml_config/iterator.hpp>

namespace yaml_config {

using Exception = formats::yaml::Exception;
using ParseException = formats::yaml::ParseException;

class YamlConfig {
 public:
  struct IterTraits {
    using value_type = YamlConfig;
    using reference = const YamlConfig&;
    using pointer = const YamlConfig*;
  };

  using const_iterator = Iterator<IterTraits>;
  using Exception = yaml_config::Exception;
  using ParseException = yaml_config::ParseException;

 public:
  YamlConfig() = default;

  YamlConfig(formats::yaml::Value yaml, formats::yaml::Value substitution_map);

  const formats::yaml::Value& Yaml() const;
  const formats::yaml::Value& SubstitutionMap() const;

  /// @brief Access member by key for read.
  /// @throw TypeMismatchException if value is not missing and is not object.
  YamlConfig operator[](std::string_view key) const;

  /// @brief Access member by index for read.
  /// @throw TypeMismatchException if value is not missing and is not array.
  YamlConfig operator[](size_t index) const;

  /// @brief Returns array size or object members count.
  /// @throw TypeMismatchException if not array or object value.
  std::size_t GetSize() const;

  /// @brief Returns true if *this holds nothing. When `IsMissing()` returns
  /// `true` any attempt to get the actual value or iterate over *this will
  /// throw MemberMissingException.
  bool IsMissing() const;

  /// @brief Returns true if *this holds 'null'.
  bool IsNull() const;

  /// @brief Returns true if *this is convertible to int64_t.
  /// @throw Nothing.
  bool IsInt64() const;

  /// @brief Returns true if *this is convertible to uint64_t.
  /// @throw Nothing.
  bool IsUInt64() const;

  /// @throw MemberMissingException if `*this` is not an array or Null.
  void CheckArrayOrNull() const;

  /// @throw TypeMismatchException if `*this` is not a map or Null.
  void CheckObjectOrNull() const;

  /// @brief Returns value of *this converted to T.
  /// @throw Anything derived from std::exception.
  template <typename T>
  T As() const;

  /// @brief Returns value of *this converted to T or T(args) if
  /// this->IsMissing().
  /// @throw Anything derived from std::exception.
  template <typename T, typename First, typename... Rest>
  T As(First&& default_arg, Rest&&... more_default_args) const;

  /// @brief Returns true if *this holds a `key`.
  /// @throw Nothing.
  bool HasMember(std::string_view key) const;

  /// @brief Returns full path to this value.
  std::string GetPath() const;

  /// @brief Returns an iterator to the beginning of the held array or map.
  /// @throw TypeMismatchException is the value of *this is not a map, array
  /// or Null.
  const_iterator begin() const;

  /// @brief Returns an iterator to the end of the held array or map.
  /// @throw TypeMismatchException is the value of *this is not a map, array
  /// or Null.
  const_iterator end() const;

  [[deprecated("Use config[name].As<int>() instead")]] int ParseInt(
      std::string_view name) const;
  [[deprecated("Use config[name].As<int>(default) instead")]] int ParseInt(
      std::string_view name, int default_value) const;
  [[deprecated(
      "Use config[name].As<std::optional<int>>() instead")]] std::optional<int>
  ParseOptionalInt(std::string_view name) const;

  [[deprecated("Use config[name].As<bool>() instead")]] bool ParseBool(
      std::string_view name) const;
  [[deprecated("Use config[name].As<bool>(default) instead")]] bool ParseBool(
      std::string_view name, bool default_value) const;
  [[deprecated("Use config[name].As<std::optional<bool>>() instead")]] std::
      optional<bool>
      ParseOptionalBool(std::string_view name) const;

  [[deprecated("Use config[name].As<uint64_t>() instead")]] uint64_t
  ParseUint64(std::string_view name) const;
  [[deprecated("Use config[name].As<uint64_t>(default) instead")]] uint64_t
  ParseUint64(std::string_view name, uint64_t default_value) const;
  [[deprecated("Use config[name].As<std::optional<uint64_t>>() instead")]] std::
      optional<uint64_t>
      ParseOptionalUint64(std::string_view name) const;

  [[deprecated("Use config[name].As<std::string>() instead")]] std::string
  ParseString(std::string_view name) const;
  [[deprecated(
      "Use config[name].As<std::string>(default) instead")]] std::string
  ParseString(std::string_view name, std::string_view default_value) const;
  [[deprecated(
      "Use config[name].As<std::optional<std::string>>() instead")]] std::
      optional<std::string>
      ParseOptionalString(std::string_view name) const;

  [[deprecated(
      "Use config[name].As<std::chrono::milliseconds>() instead")]] std::
      chrono::milliseconds
      ParseDuration(std::string_view name) const;
  [[deprecated(
      "Use config[name].As<std::chrono::milliseconds>(default) instead")]] std::
      chrono::milliseconds
      ParseDuration(std::string_view name,
                    std::chrono::milliseconds default_value) const;
  [[deprecated(
      "Use config[name].As<std::optional<std::chrono::milliseconds>>() "
      "instead")]] std::optional<std::chrono::milliseconds>
  ParseOptionalDuration(std::string_view name) const;

  template <typename T>
  [[deprecated("Use config[name].As<T>() instead")]] T Parse(
      std::string_view name) const {
    return (*this)[name].As<T>();
  }

  template <typename T>
  [[deprecated("Use config[name].As<T>() instead")]] T Parse(
      std::string_view name, T default_arg) const {
    return (*this)[name].As<T>(std::move(default_arg));
  }

 private:
  formats::yaml::Value yaml_;
  formats::yaml::Value substitution_map_;
};

namespace impl {

template <typename T>
T FormatsParse(const YamlConfig& value, formats::parse::To<T> to) {
  return Parse(value, to);
}

}  // namespace impl

template <typename T>
T YamlConfig::As() const {
  static_assert(formats::common::kHasParseTo<YamlConfig, T>,
                "There is no `Parse(const YamlConfig&, formats::parse::To<T>)`"
                "in namespace of `T` or `formats::parse`. "
                "Probably you forgot to include the "
                "<formats/parse/common_containers.hpp> or you "
                "have not provided a `Parse` function overload.");

  return impl::FormatsParse(*this, formats::parse::To<T>{});
}

template <>
bool YamlConfig::As<bool>() const;

template <>
int64_t YamlConfig::As<int64_t>() const;

template <>
uint64_t YamlConfig::As<uint64_t>() const;

template <>
double YamlConfig::As<double>() const;

template <>
std::string YamlConfig::As<std::string>() const;

template <typename T, typename First, typename... Rest>
T YamlConfig::As(First&& default_arg, Rest&&... more_default_args) const {
  if (IsMissing()) {
    return T(std::forward<First>(default_arg),
             std::forward<Rest>(more_default_args)...);
  }
  return As<T>();
}

/// @brief Wrapper for handy python-like iteration over a map
///
/// @code
///   for (const auto& [name, value]: Items(map)) ...
/// @endcode
using formats::common::Items;

/// Shorthand for parsing YamlConfig from a YAML string, mainly for tests
YamlConfig FromString(const std::string& string);

std::chrono::seconds Parse(const YamlConfig& value,
                           formats::parse::To<std::chrono::seconds>);

std::chrono::milliseconds Parse(const YamlConfig& value,
                                formats::parse::To<std::chrono::milliseconds>);

}  // namespace yaml_config
