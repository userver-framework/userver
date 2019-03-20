#include <yaml_config/value.hpp>

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

std::string GetFallbackName(const std::string& str) {
  return str + "#fallback";
}

}  // namespace impl

void CheckIsMap(const formats::yaml::Value& obj, const std::string& full_path) {
  impl::CheckIsMap(obj, full_path);
}

int ParseInt(const formats::yaml::Value& obj, const std::string& name,
             const std::string& full_path,
             const VariableMapPtr& config_vars_ptr) {
  auto optional = ParseOptionalInt(obj, name, full_path, config_vars_ptr);
  if (!optional) {
    throw ParseError(full_path, name, "int");
  }
  return *optional;
}

bool ParseBool(const formats::yaml::Value& obj, const std::string& name,
               const std::string& full_path,
               const VariableMapPtr& config_vars_ptr) {
  auto optional = ParseOptionalBool(obj, name, full_path, config_vars_ptr);
  if (!optional) {
    throw ParseError(full_path, name, "bool");
  }
  return *optional;
}

uint64_t ParseUint64(const formats::yaml::Value& obj, const std::string& name,
                     const std::string& full_path,
                     const VariableMapPtr& config_vars_ptr) {
  auto optional = ParseOptionalUint64(obj, name, full_path, config_vars_ptr);
  if (!optional) {
    throw ParseError(full_path, name, "uint64");
  }
  return *optional;
}

std::string ParseString(const formats::yaml::Value& obj,
                        const std::string& name, const std::string& full_path,
                        const VariableMapPtr& config_vars_ptr) {
  auto optional = ParseOptionalString(obj, name, full_path, config_vars_ptr);
  if (!optional) {
    throw ParseError(full_path, name, "string");
  }
  return std::move(*optional);
}

boost::optional<int> ParseOptionalInt(const formats::yaml::Value& obj,
                                      const std::string& name,
                                      const std::string& full_path,
                                      const VariableMapPtr& config_vars_ptr) {
  return ParseValue(obj, name, full_path, config_vars_ptr,
                    &impl::Parse<int, std::string>, &ParseOptionalInt);
}

boost::optional<bool> ParseOptionalBool(const formats::yaml::Value& obj,
                                        const std::string& name,
                                        const std::string& full_path,
                                        const VariableMapPtr& config_vars_ptr) {
  return ParseValue(obj, name, full_path, config_vars_ptr,
                    &impl::Parse<bool, std::string>, &ParseOptionalBool);
}

boost::optional<uint64_t> ParseOptionalUint64(
    const formats::yaml::Value& obj, const std::string& name,
    const std::string& full_path, const VariableMapPtr& config_vars_ptr) {
  return ParseValue(obj, name, full_path, config_vars_ptr,
                    &impl::Parse<uint64_t, std::string>, &ParseOptionalUint64);
}

boost::optional<std::string> ParseOptionalString(
    const formats::yaml::Value& obj, const std::string& name,
    const std::string& full_path, const VariableMapPtr& config_vars_ptr) {
  return ParseValue(obj, name, full_path, config_vars_ptr,
                    &impl::Parse<std::string, std::string>,
                    &ParseOptionalString);
}

}  // namespace yaml_config
