#pragma once

#include <cstdint>
#include <string>
#include <type_traits>

#include <boost/optional.hpp>
#include <formats/yaml.hpp>

#include <logging/log.hpp>

#include "parse.hpp"
#include "variable_map.hpp"

namespace yaml_config {
namespace impl {

bool IsSubstitution(const formats::yaml::Node& value);
std::string GetSubstitutionVarName(const formats::yaml::Node& value);
std::string GetFallbackName(const std::string& str);

}  // namespace impl

void CheckIsMap(const formats::yaml::Node& obj, const std::string& full_path);

boost::optional<int> ParseOptionalInt(const formats::yaml::Node& obj,
                                      const std::string& name,
                                      const std::string& full_path,
                                      const VariableMapPtr& config_vars_ptr);
boost::optional<bool> ParseOptionalBool(const formats::yaml::Node& obj,
                                        const std::string& name,
                                        const std::string& full_path,
                                        const VariableMapPtr& config_vars_ptr);
boost::optional<uint64_t> ParseOptionalUint64(
    const formats::yaml::Node& obj, const std::string& name,
    const std::string& full_path, const VariableMapPtr& config_vars_ptr);
boost::optional<std::string> ParseOptionalString(
    const formats::yaml::Node& obj, const std::string& name,
    const std::string& full_path, const VariableMapPtr& config_vars_ptr);

int ParseInt(const formats::yaml::Node& obj, const std::string& name,
             const std::string& full_path,
             const VariableMapPtr& config_vars_ptr);
bool ParseBool(const formats::yaml::Node& obj, const std::string& name,
               const std::string& full_path,
               const VariableMapPtr& config_vars_ptr);
uint64_t ParseUint64(const formats::yaml::Node& obj, const std::string& name,
                     const std::string& full_path,
                     const VariableMapPtr& config_vars_ptr);
std::string ParseString(const formats::yaml::Node& obj, const std::string& name,
                        const std::string& full_path,
                        const VariableMapPtr& config_vars_ptr);

template <typename T>
boost::optional<std::vector<T>> ParseOptionalArray(
    const formats::yaml::Node& obj, const std::string& name,
    const std::string& full_path, const VariableMapPtr& config_vars_ptr) {
  return ParseValue(obj, name, full_path, config_vars_ptr, &impl::ParseArray<T>,
                    &ParseOptionalArray<T>);
}

template <typename T>
std::vector<T> ParseArray(const formats::yaml::Node& obj,
                          const std::string& name, const std::string& full_path,
                          const VariableMapPtr& config_vars_ptr) {
  auto optional = ParseOptionalArray<T>(obj, name, full_path, config_vars_ptr);
  if (!optional) {
    throw ParseError(full_path, name, "array");
  }
  return std::move(*optional);
}

template <typename T>
boost::optional<std::vector<T>> ParseOptionalMapAsArray(
    const formats::yaml::Node& obj, const std::string& name,
    const std::string& full_path, const VariableMapPtr& config_vars_ptr) {
  return ParseValue(obj, name, full_path, config_vars_ptr,
                    &impl::ParseMapAsArray<T>, &ParseOptionalMapAsArray<T>);
}

template <typename T>
std::vector<T> ParseMapAsArray(const formats::yaml::Node& obj,
                               const std::string& name,
                               const std::string& full_path,
                               const VariableMapPtr& config_vars_ptr) {
  auto optional =
      ParseOptionalMapAsArray<T>(obj, name, full_path, config_vars_ptr);
  if (!optional) {
    throw ParseError(full_path, name, "map");
  }
  return std::move(*optional);
}

template <typename ElemParser, typename ConfigVarParser>
auto ParseValue(const formats::yaml::Node& obj, const std::string& name,
                const std::string& full_path,
                const VariableMapPtr& config_vars_ptr, ElemParser parse_elem,
                ConfigVarParser parse_config_var)
    -> boost::optional<decltype(parse_elem(obj, name, full_path,
                                           config_vars_ptr))> {
  static_assert(
      std::is_same<boost::optional<decltype(
                       parse_elem(obj, name, full_path, config_vars_ptr))>,
                   decltype(parse_config_var(obj, name, full_path,
                                             config_vars_ptr))>::value,
      "inconsistent result types of ElemParser and ConfigVarParser");

  if (!obj) return {};
  const auto& value = obj[name];
  if (!value) return {};

  if (impl::IsSubstitution(value)) {
    auto var_name = impl::GetSubstitutionVarName(value);
    if (config_vars_ptr && config_vars_ptr->IsDefined(var_name)) {
      const auto& res = parse_config_var(config_vars_ptr->Yaml(), var_name,
                                         "<config_vars_ptr>", config_vars_ptr);
      if (res) return res;
    }
    LOG_INFO() << "using default value for config variable '" << var_name
               << '\'';
    return ParseValue(obj, impl::GetFallbackName(name), full_path,
                      config_vars_ptr, parse_elem, parse_config_var);
  }
  return parse_elem(obj, name, full_path, config_vars_ptr);
}

}  // namespace yaml_config
