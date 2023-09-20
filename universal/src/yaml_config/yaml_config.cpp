#include <userver/yaml_config/yaml_config.hpp>

#include <fmt/format.h>
#include <boost/algorithm/string/predicate.hpp>

#include <userver/formats/yaml/serialize.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/string_to_duration.hpp>

USERVER_NAMESPACE_BEGIN

namespace yaml_config {

namespace {

bool IsSubstitution(const formats::yaml::Value& value) {
  if (!value.IsString()) return false;
  const auto& str = value.As<std::string>();
  return !str.empty() && str.front() == '$';
}

std::string GetSubstitutionVarName(const formats::yaml::Value& value) {
  return value.As<std::string>().substr(1);
}

std::string GetFallbackName(std::string_view str) {
  return std::string{str} + "#fallback";
}

std::string GetEnvName(std::string_view str) {
  return std::string{str} + "#env";
}

template <typename Field>
YamlConfig MakeMissingConfig(const YamlConfig& config, Field field) {
  const auto path = formats::common::MakeChildPath(config.GetPath(), field);
  return {formats::yaml::Value()[path], {}};
}

void AssertEnvMode(YamlConfig::Mode mode) {
  if (mode != YamlConfig::Mode::kEnvAllowed) {
    throw std::runtime_error(
        "YamlConfig was not constructed with Mode::kEnvAllowed but an attempt "
        "to read an environment variable was made");
  }
}

std::optional<formats::yaml::Value> GetFromEnvByKey(
    std::string_view key, const formats::yaml::Value& yaml,
    YamlConfig::Mode mode) {
  const auto env_name = yaml[GetEnvName(key)];
  if (!env_name.IsMissing()) {
    AssertEnvMode(mode);

    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    const auto* env_value = std::getenv(env_name.As<std::string>().c_str());
    if (env_value) {
      LOG_INFO() << "using env value for '" << key << '\'';
      return formats::yaml::FromString(env_value);
    }

    const auto fallback_name = GetFallbackName(key);
    if (yaml.HasMember(fallback_name)) {
      LOG_INFO() << "using fallback value for '" << key << '\'';
      return yaml[fallback_name];
    }
  }

  return {};
}

}  // namespace

YamlConfig::YamlConfig(formats::yaml::Value yaml,
                       formats::yaml::Value config_vars, Mode mode)
    : yaml_(std::move(yaml)),
      config_vars_(std::move(config_vars)),
      mode_(mode) {}

const formats::yaml::Value& YamlConfig::Yaml() const { return yaml_; }

YamlConfig YamlConfig::operator[](std::string_view key) const {
  if (boost::algorithm::ends_with(key, "#env")) {
    auto env_value = GetFromEnvByKey(key, yaml_, mode_);
    if (env_value) {
      // Strip substitutions off to disallow nested substitutions
      return YamlConfig{std::move(*env_value), {}, Mode::kSecure};
    }

    // Avoid parsing #env as a string
    return MakeMissingConfig(*this, key);
  }

  auto value = yaml_[key];

  if (IsSubstitution(value)) {
    const auto var_name = GetSubstitutionVarName(value);

    auto var_data = config_vars_[var_name];
    if (!var_data.IsMissing()) {
      // Strip substitutions off to disallow nested substitutions
      return YamlConfig{std::move(var_data), {}, Mode::kSecure};
    }

    auto env_value = GetFromEnvByKey(key, yaml_, mode_);
    if (env_value) {
      // Strip substitutions off to disallow nested substitutions
      return YamlConfig{std::move(*env_value), {}, Mode::kSecure};
    }

    const auto fallback_name = GetFallbackName(key);
    if (yaml_.HasMember(fallback_name)) {
      LOG_INFO() << "using fallback value for '" << key << '\'';
      // Strip substitutions off to disallow nested substitutions
      return YamlConfig{yaml_[fallback_name], {}, Mode::kSecure};
    }

    // Avoid parsing $substitution as a string
    return MakeMissingConfig(*this, key);
  }

  if (value.IsMissing()) {
    auto env_value = GetFromEnvByKey(key, yaml_, mode_);
    if (env_value) {
      // Strip substitutions off to disallow nested substitutions
      return YamlConfig{std::move(*env_value), {}, Mode::kSecure};
    }
  }

  return YamlConfig{std::move(value), config_vars_, mode_};
}

YamlConfig YamlConfig::operator[](size_t index) const {
  auto value = yaml_[index];

  if (IsSubstitution(value)) {
    const auto var_name = GetSubstitutionVarName(value);

    auto var_data = config_vars_[var_name];
    if (!var_data.IsMissing()) {
      // Strip substitutions off to disallow nested substitutions
      return YamlConfig{std::move(var_data), {}, Mode::kSecure};
    }

    // Avoid parsing $substitution as a string
    return MakeMissingConfig(*this, index);
  }

  return {std::move(value), config_vars_};
}

std::size_t YamlConfig::GetSize() const { return yaml_.GetSize(); }

bool YamlConfig::IsMissing() const noexcept { return yaml_.IsMissing(); }

bool YamlConfig::IsNull() const noexcept { return yaml_.IsNull(); }

bool YamlConfig::IsBool() const noexcept { return yaml_.IsBool(); }

bool YamlConfig::IsInt() const noexcept { return yaml_.IsInt(); }

bool YamlConfig::IsInt64() const noexcept { return yaml_.IsInt64(); }

bool YamlConfig::IsUInt64() const noexcept { return yaml_.IsUInt64(); }

bool YamlConfig::IsDouble() const noexcept { return yaml_.IsDouble(); }

bool YamlConfig::IsString() const noexcept { return yaml_.IsString(); }

bool YamlConfig::IsArray() const noexcept { return yaml_.IsArray(); }

bool YamlConfig::IsObject() const noexcept { return yaml_.IsObject(); }

void YamlConfig::CheckNotMissing() const { return yaml_.CheckNotMissing(); }

void YamlConfig::CheckArray() const { return yaml_.CheckArray(); }

void YamlConfig::CheckArrayOrNull() const { yaml_.CheckArrayOrNull(); }

void YamlConfig::CheckObjectOrNull() const { yaml_.CheckObjectOrNull(); }

void YamlConfig::CheckObject() const { yaml_.CheckObject(); }

void YamlConfig::CheckString() const { yaml_.CheckString(); }

void YamlConfig::CheckObjectOrArrayOrNull() const {
  yaml_.CheckObjectOrArrayOrNull();
}

bool YamlConfig::HasMember(std::string_view key) const {
  return yaml_.HasMember(key);
}

std::string YamlConfig::GetPath() const { return yaml_.GetPath(); }

YamlConfig::const_iterator YamlConfig::begin() const {
  return const_iterator{*this, yaml_.begin()};
}

YamlConfig::const_iterator YamlConfig::end() const {
  return const_iterator{*this, yaml_.end()};
}

template <>
bool YamlConfig::As<bool>() const {
  return yaml_.As<bool>();
}

template <>
int64_t YamlConfig::As<int64_t>() const {
  return yaml_.As<int64_t>();
}

template <>
uint64_t YamlConfig::As<uint64_t>() const {
  return yaml_.As<uint64_t>();
}

template <>
double YamlConfig::As<double>() const {
  return yaml_.As<double>();
}

template <>
std::string YamlConfig::As<std::string>() const {
  return yaml_.As<std::string>();
}

std::chrono::seconds Parse(const YamlConfig& value,
                           formats::parse::To<std::chrono::seconds>) {
  const auto as_milliseconds =
      Parse(value, formats::parse::To<std::chrono::milliseconds>{});
  const auto as_seconds =
      std::chrono::duration_cast<std::chrono::seconds>(as_milliseconds);

  if (as_seconds != as_milliseconds) {
    throw ParseException(
        fmt::format("While parsing '{}': '{}' cannot be represented "
                    "as milliseconds without precision loss",
                    value.GetPath(), value.As<std::string>()));
  }

  return as_seconds;
}

std::chrono::milliseconds Parse(const YamlConfig& value,
                                formats::parse::To<std::chrono::milliseconds>) {
  const auto as_string = value.As<std::string>();
  try {
    return utils::StringToDuration(as_string);
  } catch (const std::exception& ex) {
    throw ParseException(
        fmt::format("While parsing '{}': {}", value.GetPath(), ex.what()));
  }
}

}  // namespace yaml_config

USERVER_NAMESPACE_END
