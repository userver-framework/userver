#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/optional.hpp>

#include <compiler/demangle.hpp>
#include <formats/yaml.hpp>
#include <utils/void_t.hpp>

#include "variable_map.hpp"

namespace yaml_config {
namespace impl {

std::string PathAppend(const std::string& full_path, const std::string& name);
std::string PathAppend(const std::string& full_path, size_t idx);

}  // namespace impl

class ParseError : public std::runtime_error {
 public:
  template <typename Field>
  ParseError(const std::string& full_path, const Field& field,
             const std::string& type)
      : std::runtime_error("Cannot parse " +
                           impl::PathAppend(full_path, field) + ": " + type +
                           " expected") {}
};

namespace impl {

void CheckIsMap(const formats::yaml::Value& obj, const std::string& full_path);
void CheckIsSequence(const formats::yaml::Value& obj,
                     const std::string& full_path);

void CheckContainer(const formats::yaml::Value& obj, const std::string&,
                    const std::string& full_path);
void CheckContainer(const formats::yaml::Value& obj, const size_t&,
                    const std::string& full_path);

template <typename Type, typename Field>
Type ParseTypeImpl(const formats::yaml::Value& obj, const Field& field,
                   const std::string& full_path, const std::string& type_name) {
  const auto& value = obj[field];
  CheckContainer(obj, field, full_path);
  try {
    return value.template As<Type>();
  } catch (formats::yaml::Exception&) {
    throw ParseError(full_path, field, type_name);
  }
}

template <typename T>
static std::string GetTypeName() {
  return compiler::GetTypeName<T>();
}

template <>
inline std::string GetTypeName<std::string>() {
  static const std::string kString = "string";
  return kString;
}

template <typename Type, typename Field>
class ParseHelper {
 private:
  template <typename T, typename = ::utils::void_t<>>
  struct HasParseFromYaml : public std::false_type {};
  template <typename T>
  struct HasParseFromYaml<
      T, ::utils::void_t<decltype(T::ParseFromYaml(
             std::declval<formats::yaml::Value>(), std::declval<std::string>(),
             std::declval<yaml_config::VariableMapPtr>()))>>
      : public std::true_type {};

 public:
  template <typename T = Type>
  std::enable_if_t<HasParseFromYaml<T>::value, T> operator()(
      const formats::yaml::Value& obj, const Field& field,
      const std::string& full_path,
      const VariableMapPtr& config_vars_ptr) const {
    const auto& value = obj[field];
    return T::ParseFromYaml(value, PathAppend(full_path, field),
                            config_vars_ptr);
  }

  template <typename T = Type>
  std::enable_if_t<!HasParseFromYaml<T>::value, T> operator()(
      const formats::yaml::Value& obj, const Field& field,
      const std::string& full_path, const VariableMapPtr&) const {
    return ParseTypeImpl<T>(obj, field, full_path, GetTypeName<T>());
  }
};

template <typename T, typename Field>
T Parse(const formats::yaml::Value& obj, const Field& field,
        const std::string& full_path, const VariableMapPtr& config_vars_ptr);

template <typename T, typename Field>
inline boost::optional<std::vector<T>> ParseOptionalArray(
    const formats::yaml::Value& obj, const Field& field,
    const std::string& full_path, const VariableMapPtr& config_vars_ptr) {
  const auto& value = obj[field];
  if (!value.IsArray()) {
    return {};
  }

  std::vector<T> parsed_array;
  auto size = value.GetSize();
  parsed_array.reserve(size);
  for (decltype(size) i = 0; i < size; ++i) {
    // TODO: "$substitutions" are not supported for array elements yet
    parsed_array.emplace_back(impl::Parse<T>(
        value, i, PathAppend(full_path, field), config_vars_ptr));
  }
  return parsed_array;
}

template <typename T, typename Field>
inline std::vector<T> ParseArray(const formats::yaml::Value& obj,
                                 const Field& field,
                                 const std::string& full_path,
                                 const VariableMapPtr& config_vars_ptr) {
  auto parsed_optional =
      impl::ParseOptionalArray<T>(obj, field, full_path, config_vars_ptr);
  if (!parsed_optional) {
    throw ParseError(full_path, field, "array");
  }
  return std::move(*parsed_optional);
}

template <typename T>
inline boost::optional<std::vector<T>> ParseOptionalMapAsArray(
    const formats::yaml::Value& obj, const std::string& name,
    const std::string& full_path, const VariableMapPtr& config_vars_ptr) {
  const auto& value = obj[name];
  if (!value.IsObject()) {
    return {};
  }

  std::vector<T> parsed_array;
  parsed_array.reserve(value.GetSize());
  for (auto it = value.begin(); it != value.end(); ++it) {
    const auto elem_name = it.GetName();
    auto parsed = T::ParseFromYaml(
        // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
        *it, full_path + '.' + name + '.' + elem_name, config_vars_ptr);
    parsed.SetName(elem_name);
    parsed_array.emplace_back(std::move(parsed));
  }
  return parsed_array;
}

template <typename T>
inline std::vector<T> ParseMapAsArray(const formats::yaml::Value& obj,
                                      const std::string& name,
                                      const std::string& full_path,
                                      const VariableMapPtr& config_vars_ptr) {
  auto parsed_optional =
      impl::ParseOptionalMapAsArray<T>(obj, name, full_path, config_vars_ptr);
  if (!parsed_optional) {
    throw ParseError(full_path, name, "map");
  }
  return std::move(*parsed_optional);
}

template <typename T, typename Field>
struct ParseHelperArray {
  T operator()(const formats::yaml::Value& obj, const Field& field,
               const std::string& full_path,
               const VariableMapPtr& config_vars_ptr) const {
    return ParseHelper<T, Field>()(obj, field, full_path, config_vars_ptr);
  }
};

template <typename T, typename Field>
struct ParseHelperArray<std::vector<T>, Field> {
  std::vector<T> operator()(const formats::yaml::Value& obj, const Field& field,
                            const std::string& full_path,
                            const VariableMapPtr& config_vars_ptr) const {
    return impl::ParseArray<T>(obj, field, full_path, config_vars_ptr);
  }
};

template <typename T, typename Field>
struct ParseHelperOptional {
  T operator()(const formats::yaml::Value& obj, const Field& field,
               const std::string& full_path,
               const VariableMapPtr& config_vars_ptr) const {
    return ParseHelperArray<T, Field>()(obj, field, full_path, config_vars_ptr);
  }
};

template <typename T, typename Field>
struct ParseHelperOptional<boost::optional<T>, Field> {
  boost::optional<T> operator()(const formats::yaml::Value& obj,
                                const Field& field,
                                const std::string& full_path,
                                const VariableMapPtr& config_vars_ptr) const {
    const auto& value = obj[field];
    if (!value || value.IsNull()) {
      return {};
    }
    return Parse<T>(obj, field, full_path, config_vars_ptr);
  }
};

template <typename T, typename Field>
T Parse(const formats::yaml::Value& obj, const Field& field,
        const std::string& full_path, const VariableMapPtr& config_vars_ptr) {
  return ParseHelperOptional<T, Field>()(obj, field, full_path,
                                         config_vars_ptr);
}

}  // namespace impl
}  // namespace yaml_config
