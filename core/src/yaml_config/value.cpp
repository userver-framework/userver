#include <yaml_config/value.hpp>

#include <fmt/compile.h>

namespace yaml_config {
namespace impl {

bool IsSubstitution(const formats::yaml::Value& value) {
  try {
    const auto& str = value.As<std::string>();
    return !str.empty() && str.front() == '$';
  } catch (formats::yaml::Exception&) {
    return false;
  }
}

std::string GetSubstitutionVarName(const formats::yaml::Value& value) {
  return value.As<std::string>().substr(1);
}

std::string GetFallbackName(std::string_view str) {
  return fmt::format(FMT_COMPILE("{}#fallback"), str);
}

}  // namespace impl

void CheckIsMap(const formats::yaml::Value& obj, std::string_view full_path) {
  impl::CheckIsMap(obj, full_path);
}

int ParseInt(const formats::yaml::Value& obj, std::string_view name,
             std::string_view full_path,
             const VariableMapPtr& config_vars_ptr) {
  auto optional = ParseOptionalInt(obj, name, full_path, config_vars_ptr);
  if (!optional) {
    throw ParseError(full_path, name, "int");
  }
  return *optional;
}

bool ParseBool(const formats::yaml::Value& obj, std::string_view name,
               std::string_view full_path,
               const VariableMapPtr& config_vars_ptr) {
  auto optional = ParseOptionalBool(obj, name, full_path, config_vars_ptr);
  if (!optional) {
    throw ParseError(full_path, name, "bool");
  }
  return *optional;
}

uint64_t ParseUint64(const formats::yaml::Value& obj, std::string_view name,
                     std::string_view full_path,
                     const VariableMapPtr& config_vars_ptr) {
  auto optional = ParseOptionalUint64(obj, name, full_path, config_vars_ptr);
  if (!optional) {
    throw ParseError(full_path, name, "uint64");
  }
  return *optional;
}

std::string ParseString(const formats::yaml::Value& obj, std::string_view name,
                        std::string_view full_path,
                        const VariableMapPtr& config_vars_ptr) {
  auto optional = ParseOptionalString(obj, name, full_path, config_vars_ptr);
  if (!optional) {
    throw ParseError(full_path, name, "string");
  }
  return std::move(*optional);
}

std::optional<int> ParseOptionalInt(const formats::yaml::Value& obj,
                                    std::string_view name,
                                    std::string_view full_path,
                                    const VariableMapPtr& config_vars_ptr) {
  return ParseValue(obj, name, full_path, config_vars_ptr,
                    &impl::Parse<int, std::string_view>, &ParseOptionalInt);
}

std::optional<bool> ParseOptionalBool(const formats::yaml::Value& obj,
                                      std::string_view name,
                                      std::string_view full_path,
                                      const VariableMapPtr& config_vars_ptr) {
  return ParseValue(obj, name, full_path, config_vars_ptr,
                    &impl::Parse<bool, std::string_view>, &ParseOptionalBool);
}

std::optional<uint64_t> ParseOptionalUint64(
    const formats::yaml::Value& obj, std::string_view name,
    std::string_view full_path, const VariableMapPtr& config_vars_ptr) {
  return ParseValue(obj, name, full_path, config_vars_ptr,
                    &impl::Parse<uint64_t, std::string_view>,
                    &ParseOptionalUint64);
}

std::optional<std::string> ParseOptionalString(
    const formats::yaml::Value& obj, std::string_view name,
    std::string_view full_path, const VariableMapPtr& config_vars_ptr) {
  return ParseValue(obj, name, full_path, config_vars_ptr,
                    &impl::Parse<std::string, std::string_view>,
                    &ParseOptionalString);
}

}  // namespace yaml_config
