#pragma once

/// @file userver/yaml_config/yaml_config.hpp
/// @brief @copybrief yaml_config::YamlConfig

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <userver/formats/parse/common.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/yaml/value.hpp>

#include <userver/yaml_config/iterator.hpp>

USERVER_NAMESPACE_BEGIN

/// Utilities to work with static YAML config
namespace yaml_config {

using Exception = formats::yaml::Exception;
using ParseException = formats::yaml::ParseException;

/// @ingroup userver_formats
///
/// @brief Datatype that represents YAML with substituted variables
class YamlConfig {
 public:
  struct IterTraits {
    using value_type = YamlConfig;
    using reference = const YamlConfig&;
    using pointer = const YamlConfig*;
  };
  struct DefaultConstructed {};

  using const_iterator = Iterator<IterTraits>;
  using Exception = yaml_config::Exception;
  using ParseException = yaml_config::ParseException;

  YamlConfig() = default;

  /// YamlConfig = config + config_vars
  YamlConfig(formats::yaml::Value yaml, formats::yaml::Value config_vars);

  /// Get the plain Yaml without substitutions. It may contain raw references.
  const formats::yaml::Value& Yaml() const;

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
  bool IsMissing() const noexcept;

  /// @brief Returns true if *this holds 'null'.
  bool IsNull() const noexcept;

  /// @brief Returns true if *this is convertible to int64_t.
  bool IsInt64() const noexcept;

  /// @brief Returns true if *this is convertible to uint64_t.
  bool IsUInt64() const noexcept;

  /// @brief Returns true if *this is convertible to std::string.
  bool IsString() const noexcept;

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

  /// @brief Returns value of *this converted to T or T() if this->IsMissing().
  /// @throw Anything derived from std::exception.
  /// @note Use as `value.As<T>({})`
  template <typename T>
  T As(DefaultConstructed) const;

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

 private:
  formats::yaml::Value yaml_;
  formats::yaml::Value config_vars_;
};

template <typename T>
T YamlConfig::As() const {
  static_assert(formats::common::impl::kHasParse<YamlConfig, T>,
                "There is no `Parse(const YamlConfig&, formats::parse::To<T>)`"
                "in namespace of `T` or `formats::parse`. "
                "Probably you forgot to include the "
                "<userver/formats/parse/common_containers.hpp> or you "
                "have not provided a `Parse` function overload.");

  return Parse(*this, formats::parse::To<T>{});
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
    // intended raw ctor call, sometimes casts
    // NOLINTNEXTLINE(google-readability-casting)
    return T(std::forward<First>(default_arg),
             std::forward<Rest>(more_default_args)...);
  }
  return As<T>();
}

template <typename T>
T YamlConfig::As(YamlConfig::DefaultConstructed) const {
  return IsMissing() ? T() : As<T>();
}

/// @brief Wrapper for handy python-like iteration over a map
///
/// @code
///   for (const auto& [name, value]: Items(map)) ...
/// @endcode
using formats::common::Items;

/// @brief Parses duration from string, understands suffixes: ms, s, m, h, d
/// @throws On invalid type, invalid string format, and if the duration is not a
/// whole amount of seconds
std::chrono::seconds Parse(const YamlConfig& value,
                           formats::parse::To<std::chrono::seconds>);

/// @brief Parses duration from string, understands suffixes: ms, s, m, h, d
/// @throws On invalid type and invalid string format
std::chrono::milliseconds Parse(const YamlConfig& value,
                                formats::parse::To<std::chrono::milliseconds>);

}  // namespace yaml_config

USERVER_NAMESPACE_END
