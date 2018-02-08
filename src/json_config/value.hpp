#pragma once

#include <cstdint>
#include <string>
#include <type_traits>

#include <json/json.h>
#include <boost/optional.hpp>

#include <logger/logger.hpp>

#include "parse.hpp"
#include "variable_map.hpp"

namespace json_config {
namespace impl {

bool IsSubstitution(const Json::Value& value);
std::string GetSubstitutionVarName(const Json::Value& value);
std::string GetFallbackName(const std::string& str);

}  // namespace impl

boost::optional<int> ParseOptionalInt(const Json::Value& obj,
                                      const std::string& name,
                                      const std::string& full_path,
                                      const VariableMap& config_vars);
boost::optional<bool> ParseOptionalBool(const Json::Value& obj,
                                        const std::string& name,
                                        const std::string& full_path,
                                        const VariableMap& config_vars);
boost::optional<uint64_t> ParseOptionalUint64(const Json::Value& obj,
                                              const std::string& name,
                                              const std::string& full_path,
                                              const VariableMap& config_vars);
boost::optional<std::string> ParseOptionalString(
    const Json::Value& obj, const std::string& name,
    const std::string& full_path, const VariableMap& config_vars);

int ParseInt(const Json::Value& obj, const std::string& name,
             const std::string& full_path, const VariableMap& config_vars);
bool ParseBool(const Json::Value& obj, const std::string& name,
               const std::string& full_path, const VariableMap& config_vars);
uint64_t ParseUint64(const Json::Value& obj, const std::string& name,
                     const std::string& full_path,
                     const VariableMap& config_vars);
std::string ParseString(const Json::Value& obj, const std::string& name,
                        const std::string& full_path,
                        const VariableMap& config_vars);

template <typename T>
boost::optional<std::vector<T>> ParseOptionalArray(
    const Json::Value& obj, const std::string& name,
    const std::string& full_path, const VariableMap& config_vars) {
  return ParseValue(obj, name, full_path, config_vars, &impl::ParseArray<T>,
                    &ParseOptionalArray<T>);
}

template <typename T>
std::vector<T> ParseArray(const Json::Value& obj, const std::string& name,
                          const std::string& full_path,
                          const VariableMap& config_vars) {
  auto optional = ParseOptionalArray<T>(obj, name, full_path, config_vars);
  if (!optional) {
    throw ParseError(full_path, name, "array");
  }
  return std::move(*optional);
}

template <typename ElemParser, typename ConfigVarParser>
auto ParseValue(const Json::Value& obj, const std::string& name,
                const std::string& full_path, const VariableMap& config_vars,
                ElemParser parse_elem, ConfigVarParser parse_config_var)
    -> boost::optional<decltype(parse_elem(obj, name, full_path,
                                           config_vars))> {
  static_assert(std::is_same<boost::optional<decltype(parse_elem(
                                 obj, name, full_path, config_vars))>,
                             decltype(parse_config_var(obj, name, full_path,
                                                       config_vars))>::value,
                "inconsistent result types of ElemParser and ConfigVarParser");

  const auto& value = obj[name];
  if (impl::IsSubstitution(value)) {
    auto var_name = impl::GetSubstitutionVarName(value);
    if (config_vars.IsDefined(var_name)) {
      const auto& res = parse_config_var(config_vars.Json(), var_name,
                                         "<config_vars>", config_vars);
      if (res) return res;
    }
    LOG_WARNING() << "undefined config variable '" << var_name << '\'';
    return ParseValue(obj, impl::GetFallbackName(name), full_path, config_vars,
                      parse_elem, parse_config_var);
  }
  if (value.isNull()) return {};
  return parse_elem(obj, name, full_path, config_vars);
}

}  // namespace json_config
