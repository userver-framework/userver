#pragma once

/// @file userver/yaml_config/yaml_config.hpp
/// @brief @copybrief yaml_config::YamlConfig

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <userver/formats/json_fwd.hpp>
#include <userver/formats/parse/common.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/yaml/value.hpp>

#include <userver/yaml_config/iterator.hpp>

USERVER_NAMESPACE_BEGIN

/// Utilities to work with static YAML config
namespace yaml_config {

using Exception = formats::yaml::Exception;
using ParseException = formats::yaml::ParseException;

/// @ingroup userver_formats userver_universal
///
/// @brief Datatype that represents YAML with substituted variables
///
/// If YAML has value that starts with an `$`, then such value is treated as
/// a variable from `config_vars`. For example if `config_vars` contains
/// `variable: 42` and the YAML is following:
/// @snippet universal/src/yaml_config/yaml_config_test.cpp  sample vars
/// Then the result of `yaml["some_element"]["some"].As<int>()` is `42`.
///
/// If YAML key ends on '#env' and the mode is YamlConfig::Mode::kEnvAllowed,
/// then the value of the key is searched in
/// environment variables of the process and returned as a value. For example:
/// @snippet universal/src/yaml_config/yaml_config_test.cpp  sample env
///
/// If YAML key ends on '#fallback', then the value of the key is used as a
/// fallback for environment and `$` variables. For example for the following
/// YAML with YamlConfig::Mode::kEnvAllowed:
/// @snippet universal/src/yaml_config/yaml_config_test.cpp  sample multiple
/// The result of `yaml["some_element"]["some"].As<int>()` is the value of
/// `variable` from `config_vars` if it exists; otherwise the value is the
/// contents of the environment variable `SOME_ENV_VARIABLE` if it exists;
/// otherwise the value if `100500`, from the fallback.
///
/// Another example:
/// @snippet universal/src/yaml_config/yaml_config_test.cpp  sample env fallback
/// With YamlConfig::Mode::kEnvAllowed the result of
/// `yaml["some_element"]["value"].As<int>()` is the value of `ENV_NAME`
/// environment variable if it exists; otherwise it is `5`.
///
/// @warning YamlConfig::Mode::kEnvAllowed should be used only on configs that
/// come from trusted environments. Otherwise, an attacker could create a
/// config with `#env` and read any of your environment variables, including
/// variables that contain passwords and other sensitive data.
class YamlConfig {
 public:
  struct IterTraits {
    using value_type = YamlConfig;
    using reference = const YamlConfig&;
    using pointer = const YamlConfig*;
  };
  struct DefaultConstructed {};

  enum class Mode {
    kSecure,      /// < secure mode, without reading environment variables
    kEnvAllowed,  /// < allows reading of environment variables
  };

  using const_iterator = Iterator<IterTraits>;
  using Exception = yaml_config::Exception;
  using ParseException = yaml_config::ParseException;

  YamlConfig() = default;

  /// YamlConfig = config + config_vars
  YamlConfig(formats::yaml::Value yaml, formats::yaml::Value config_vars,
             Mode mode = Mode::kSecure);

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

  /// @brief Returns value of *this converted to T.
  /// @throw Anything derived from std::exception.
  template <typename T>
  auto As() const;

  /// @brief Returns value of *this converted to T or T(args) if
  /// this->IsMissing().
  /// @throw Anything derived from std::exception.
  template <typename T, typename First, typename... Rest>
  auto As(First&& default_arg, Rest&&... more_default_args) const;

  /// @brief Returns value of *this converted to T or T() if this->IsMissing().
  /// @throw Anything derived from std::exception.
  /// @note Use as `value.As<T>({})`
  template <typename T>
  auto As(DefaultConstructed) const;

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
  Mode mode_{Mode::kSecure};

  friend bool Parse(const YamlConfig& value, formats::parse::To<bool>);
  friend int64_t Parse(const YamlConfig& value, formats::parse::To<int64_t>);
  friend uint64_t Parse(const YamlConfig& value, formats::parse::To<uint64_t>);
  friend double Parse(const YamlConfig& value, formats::parse::To<double>);
  friend std::string Parse(const YamlConfig& value,
                           formats::parse::To<std::string>);
};

using Value = YamlConfig;

template <typename T>
auto YamlConfig::As() const {
  static_assert(formats::common::impl::kHasParse<YamlConfig, T>,
                "There is no `Parse(const yaml_config::YamlConfig&, "
                "formats::parse::To<T>)`"
                "in namespace of `T` or `formats::parse`. "
                "Probably you forgot to include the "
                "<userver/formats/parse/common_containers.hpp> or you "
                "have not provided a `Parse` function overload.");

  return Parse(*this, formats::parse::To<T>{});
}

bool Parse(const YamlConfig& value, formats::parse::To<bool>);

int64_t Parse(const YamlConfig& value, formats::parse::To<int64_t>);

uint64_t Parse(const YamlConfig& value, formats::parse::To<uint64_t>);

double Parse(const YamlConfig& value, formats::parse::To<double>);

std::string Parse(const YamlConfig& value, formats::parse::To<std::string>);

template <typename T, typename First, typename... Rest>
auto YamlConfig::As(First&& default_arg, Rest&&... more_default_args) const {
  if (IsMissing()) {
    // intended raw ctor call, sometimes casts
    // NOLINTNEXTLINE(google-readability-casting)
    return decltype(As<T>())(std::forward<First>(default_arg),
                             std::forward<Rest>(more_default_args)...);
  }
  return As<T>();
}

template <typename T>
auto YamlConfig::As(YamlConfig::DefaultConstructed) const {
  return IsMissing() ? decltype(As<T>())() : As<T>();
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

/// @brief Converts YAML to JSON
/// @throws formats::json::Value::Exception if `value.IsMissing()`
formats::json::Value Parse(const YamlConfig& value,
                           formats::parse::To<formats::json::Value>);

}  // namespace yaml_config

USERVER_NAMESPACE_END
