#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>

#include <boost/optional/optional_fwd.hpp>
#include <formats/yaml.hpp>

#include <logging/log.hpp>

#include "parse.hpp"
#include "variable_map.hpp"

namespace yaml_config {
namespace impl {

bool IsSubstitution(const formats::yaml::Value& value);
std::string GetSubstitutionVarName(const formats::yaml::Value& value);
std::string GetFallbackName(const std::string& str);

}  // namespace impl

void CheckIsMap(const formats::yaml::Value& obj, const std::string& full_path);

std::optional<int> ParseOptionalInt(const formats::yaml::Value& obj,
                                    const std::string& name,
                                    const std::string& full_path,
                                    const VariableMapPtr& config_vars_ptr);
std::optional<bool> ParseOptionalBool(const formats::yaml::Value& obj,
                                      const std::string& name,
                                      const std::string& full_path,
                                      const VariableMapPtr& config_vars_ptr);
std::optional<uint64_t> ParseOptionalUint64(
    const formats::yaml::Value& obj, const std::string& name,
    const std::string& full_path, const VariableMapPtr& config_vars_ptr);
std::optional<std::string> ParseOptionalString(
    const formats::yaml::Value& obj, const std::string& name,
    const std::string& full_path, const VariableMapPtr& config_vars_ptr);

int ParseInt(const formats::yaml::Value& obj, const std::string& name,
             const std::string& full_path,
             const VariableMapPtr& config_vars_ptr);
bool ParseBool(const formats::yaml::Value& obj, const std::string& name,
               const std::string& full_path,
               const VariableMapPtr& config_vars_ptr);
uint64_t ParseUint64(const formats::yaml::Value& obj, const std::string& name,
                     const std::string& full_path,
                     const VariableMapPtr& config_vars_ptr);
std::string ParseString(const formats::yaml::Value& obj,
                        const std::string& name, const std::string& full_path,
                        const VariableMapPtr& config_vars_ptr);

template <typename T, typename Field>
std::optional<T> ParseOptional(const formats::yaml::Value& obj,
                               const Field& field, const std::string& full_path,
                               const VariableMapPtr& config_vars_ptr) {
  return ParseValue(obj, field, full_path, config_vars_ptr,
                    &impl::Parse<T, Field>, &ParseOptional<T, std::string>);
}

namespace impl {

template <typename T>
struct ParseOptionalHelper {
  T operator()(const formats::yaml::Value& obj, const std::string& name,
               const std::string& full_path,
               const VariableMapPtr& config_vars_ptr) const {
    auto optional = ParseOptional<T>(obj, name, full_path, config_vars_ptr);
    if (!optional) {
      throw ParseError(full_path, name, "not missing value");
    }
    return std::move(*optional);
  }
};

template <typename T>
struct ParseOptionalHelper<boost::optional<T>> {
  static boost::optional<T> ParseOptional(
      const formats::yaml::Value& obj, const std::string& name,
      const std::string& full_path, const VariableMapPtr& config_vars_ptr) {
    return ParseValue(obj, name, full_path, config_vars_ptr,
                      &impl::Parse<T, std::string>, &ParseOptional);
  }

  boost::optional<T> operator()(const formats::yaml::Value& obj,
                                const std::string& name,
                                const std::string& full_path,
                                const VariableMapPtr& config_vars_ptr) const {
    return ParseOptional(obj, name, full_path, config_vars_ptr);
  }
};

template <typename T>
struct ParseOptionalHelper<std::optional<T>> {
  std::optional<T> operator()(const formats::yaml::Value& obj,
                              const std::string& name,
                              const std::string& full_path,
                              const VariableMapPtr& config_vars_ptr) const {
    return ParseOptional<T>(obj, name, full_path, config_vars_ptr);
  }
};

}  // namespace impl

template <typename T>
T Parse(const formats::yaml::Value& obj, const std::string& name,
        const std::string& full_path, const VariableMapPtr& config_vars_ptr) {
  return impl::ParseOptionalHelper<T>()(obj, name, full_path, config_vars_ptr);
}

template <typename T>
void ParseInto(T& result, const formats::yaml::Value& obj,
               const std::string& name, const std::string& full_path,
               const VariableMapPtr& config_vars_ptr) {
  result = Parse<T>(obj, name, full_path, config_vars_ptr);
}

template <typename T>
std::optional<std::vector<T>> ParseOptionalArray(
    const formats::yaml::Value& obj, const std::string& name,
    const std::string& full_path, const VariableMapPtr& config_vars_ptr) {
  return ParseValue(obj, name, full_path, config_vars_ptr, &impl::ParseArray<T>,
                    &ParseOptionalArray<T>);
}

template <typename T>
std::vector<T> ParseArray(const formats::yaml::Value& obj,
                          const std::string& name, const std::string& full_path,
                          const VariableMapPtr& config_vars_ptr) {
  auto optional = ParseOptionalArray<T>(obj, name, full_path, config_vars_ptr);
  if (!optional) {
    throw ParseError(full_path, name, "array");
  }
  return std::move(*optional);
}

template <typename T>
std::optional<std::vector<T>> ParseOptionalMapAsArray(
    const formats::yaml::Value& obj, const std::string& name,
    const std::string& full_path, const VariableMapPtr& config_vars_ptr) {
  return ParseValue(obj, name, full_path, config_vars_ptr,
                    &impl::ParseMapAsArray<T>, &ParseOptionalMapAsArray<T>);
}

template <typename T>
std::vector<T> ParseMapAsArray(const formats::yaml::Value& obj,
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

template <typename Field, typename ElemParser, typename ConfigVarParser>
auto ParseValue(const formats::yaml::Value& obj, const Field& field,
                const std::string& full_path,
                const VariableMapPtr& config_vars_ptr, ElemParser parse_elem,
                ConfigVarParser parse_config_var)
    -> decltype(parse_config_var(obj, std::declval<std::string>(), full_path,
                                 config_vars_ptr)) {
  using ResultType = decltype(parse_config_var(obj, std::declval<std::string>(),
                                               full_path, config_vars_ptr));
  using ParseElemType =
      decltype(parse_elem(obj, field, full_path, config_vars_ptr));

  static_assert(
      std::is_same<boost::optional<ParseElemType>, ResultType>::value ||
          std::is_same<std::optional<ParseElemType>, ResultType>::value,
      "inconsistent result types of ElemParser and ConfigVarParser");

  if (obj.IsMissing()) return {};
  const auto& value = obj[field];
  if (value.IsMissing()) return {};

  if (impl::IsSubstitution(value)) {
    auto var_name = impl::GetSubstitutionVarName(value);
    if (config_vars_ptr && config_vars_ptr->IsDefined(var_name)) {
      const auto& res = parse_config_var(config_vars_ptr->Yaml(), var_name,
                                         "<config_vars_ptr>", config_vars_ptr);
      if (res) return res;
    }
    if constexpr (std::is_same_v<Field, size_t>) {
      // no fallback for an array element with $substitution
      return {};
    } else {
      LOG_INFO() << "using default value for config variable '" << var_name
                 << '\'';
      return ParseValue(obj, impl::GetFallbackName(field), full_path,
                        config_vars_ptr, parse_elem, parse_config_var);
    }
  }
  return parse_elem(obj, field, full_path, config_vars_ptr);
}

}  // namespace yaml_config
