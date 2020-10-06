#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include <compiler/demangle.hpp>
#include <formats/common/path.hpp>
#include <formats/yaml.hpp>
#include <utils/meta.hpp>
#include <utils/void_t.hpp>

#include "variable_map.hpp"

namespace yaml_config {

class ParseError : public std::runtime_error {
 public:
  template <typename Field>
  ParseError(std::string_view full_path, Field field, std::string_view type)
      : std::runtime_error(fmt::format(
            FMT_STRING("Cannot parse {}: {} expected"),
            formats::common::MakeChildPath(full_path, field), type)) {}
};

namespace impl {

void CheckIsMap(const formats::yaml::Value& obj, std::string_view full_path);
void CheckIsSequence(const formats::yaml::Value& obj,
                     std::string_view full_path);

void CheckContainer(const formats::yaml::Value& obj, std::string_view,
                    std::string_view full_path);
void CheckContainer(const formats::yaml::Value& obj, size_t,
                    std::string_view full_path);

template <typename Type, typename Field>
Type ParseTypeImpl(const formats::yaml::Value& obj, Field field,
                   std::string_view full_path, std::string_view type_name) {
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
class ParseHelper final {
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
      const formats::yaml::Value& obj, Field field, std::string_view full_path,
      const VariableMapPtr& config_vars_ptr) const {
    const auto& value = obj[field];
    return T::ParseFromYaml(value,
                            formats::common::MakeChildPath(full_path, field),
                            config_vars_ptr);
  }

  template <typename T = Type>
  std::enable_if_t<!HasParseFromYaml<T>::value, T> operator()(
      const formats::yaml::Value& obj, Field field, std::string_view full_path,
      const VariableMapPtr&) const {
    return ParseTypeImpl<T>(obj, field, full_path, GetTypeName<T>());
  }
};

template <typename T, typename Field>
T Parse(const formats::yaml::Value& obj, Field field,
        std::string_view full_path, const VariableMapPtr& config_vars_ptr);

template <typename T, typename Field>
inline std::optional<std::vector<T>> ParseOptionalArray(
    const formats::yaml::Value& obj, Field field, std::string_view full_path,
    const VariableMapPtr& config_vars_ptr) {
  const auto& value = obj[field];
  if (!value.IsArray()) {
    return {};
  }

  std::vector<T> parsed_array;
  auto size = value.GetSize();
  parsed_array.reserve(size);
  if (size) {
    auto value_path = formats::common::MakeChildPath(full_path, field);
    for (decltype(size) i = 0; i < size; ++i) {
      auto elem = ParseValue(value, i, value_path, config_vars_ptr,
                             &impl::Parse<T, size_t>,
                             &impl::Parse<std::optional<T>, std::string>);
      if (elem) {
        parsed_array.emplace_back(std::move(*elem));
      } else {
        if constexpr (meta::kIsOptional<T>)
          parsed_array.emplace_back(std::nullopt);
        else
          throw ParseError(value_path, i, "declared config_var element");
      }
    }
  }
  return parsed_array;
}

template <typename T, typename Field>
inline std::vector<T> ParseArray(const formats::yaml::Value& obj, Field field,
                                 std::string_view full_path,
                                 const VariableMapPtr& config_vars_ptr) {
  auto parsed_optional =
      impl::ParseOptionalArray<T>(obj, field, full_path, config_vars_ptr);
  if (!parsed_optional) {
    throw ParseError(full_path, field, "array");
  }
  return std::move(*parsed_optional);
}

template <typename T>
inline std::optional<std::vector<T>> ParseOptionalMapAsArray(
    const formats::yaml::Value& obj, std::string_view name,
    std::string_view full_path, const VariableMapPtr& config_vars_ptr) {
  const auto& value = obj[name];
  if (!value.IsObject()) {
    return {};
  }

  std::vector<T> parsed_array;
  parsed_array.reserve(value.GetSize());
  for (const auto& [elem_name, yaml] : Items(value)) {
    auto parsed = T::ParseFromYaml(
        yaml, fmt::format(FMT_STRING("{}.{}.{}"), full_path, name, elem_name),
        config_vars_ptr);
    parsed.SetName(elem_name);
    parsed_array.emplace_back(std::move(parsed));
  }
  return parsed_array;
}

template <typename T>
inline std::vector<T> ParseMapAsArray(const formats::yaml::Value& obj,
                                      std::string_view name,
                                      std::string_view full_path,
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
  T operator()(const formats::yaml::Value& obj, Field field,
               std::string_view full_path,
               const VariableMapPtr& config_vars_ptr) const {
    return ParseHelper<T, Field>()(obj, field, full_path, config_vars_ptr);
  }
};

template <typename T, typename Field>
struct ParseHelperArray<std::vector<T>, Field> {
  std::vector<T> operator()(const formats::yaml::Value& obj, Field field,
                            std::string_view full_path,
                            const VariableMapPtr& config_vars_ptr) const {
    return impl::ParseArray<T>(obj, field, full_path, config_vars_ptr);
  }
};

template <typename T, typename Field>
struct ParseHelperOptional {
  T operator()(const formats::yaml::Value& obj, Field field,
               std::string_view full_path,
               const VariableMapPtr& config_vars_ptr) const {
    return ParseHelperArray<T, Field>()(obj, field, full_path, config_vars_ptr);
  }
};

template <typename T, typename Field>
struct ParseHelperOptional<std::optional<T>, Field> {
  std::optional<T> operator()(const formats::yaml::Value& obj, Field field,
                              std::string_view full_path,
                              const VariableMapPtr& config_vars_ptr) const {
    const auto& value = obj[field];
    if (value.IsMissing() || value.IsNull()) {
      return {};
    }
    return Parse<T>(obj, field, full_path, config_vars_ptr);
  }
};

template <typename T, typename Field>
T Parse(const formats::yaml::Value& obj, Field field,
        std::string_view full_path, const VariableMapPtr& config_vars_ptr) {
  return ParseHelperOptional<T, Field>()(obj, field, full_path,
                                         config_vars_ptr);
}

}  // namespace impl
}  // namespace yaml_config
