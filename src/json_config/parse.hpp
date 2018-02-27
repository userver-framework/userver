#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <json/json.h>
#include <boost/optional.hpp>

#include "variable_map.hpp"

namespace json_config {

class ParseError : public std::runtime_error {
 public:
  ParseError(const std::string& full_path, const std::string& name,
             const std::string& type);
};

namespace impl {

void CheckIsObject(const Json::Value& obj, const std::string& full_path);

boost::optional<int> ParseOptionalInt(const Json::Value& obj,
                                      const std::string& name,
                                      const std::string& full_path);
boost::optional<bool> ParseOptionalBool(const Json::Value& obj,
                                        const std::string& name,
                                        const std::string& full_path);
boost::optional<uint64_t> ParseOptionalUint64(const Json::Value& obj,
                                              const std::string& name,
                                              const std::string& full_path);
boost::optional<std::string> ParseOptionalString(const Json::Value& obj,
                                                 const std::string& name,
                                                 const std::string& full_path);

int ParseInt(const Json::Value& obj, const std::string& name,
             const std::string& full_path);
bool ParseBool(const Json::Value& obj, const std::string& name,
               const std::string& full_path);
uint64_t ParseUint64(const Json::Value& obj, const std::string& name,
                     const std::string& full_path);
std::string ParseString(const Json::Value& obj, const std::string& name,
                        const std::string& full_path);

template <typename T>
inline boost::optional<std::vector<T>> ParseOptionalArray(
    const Json::Value& obj, const std::string& name,
    const std::string& full_path, const VariableMapPtr& config_vars_ptr) {
  const auto& value = obj[name];
  if (!value.isArray()) {
    return {};
  }

  std::vector<T> parsed_array;
  auto size = value.size();
  parsed_array.reserve(size);
  for (decltype(size) i = 0; i < size; ++i) {
    parsed_array.emplace_back(T::ParseFromJson(
        value[i], full_path + '.' + name + '[' + std::to_string(i) + ']',
        config_vars_ptr));
  }
  return std::move(parsed_array);
}

template <typename T>
inline std::vector<T> ParseArray(const Json::Value& obj,
                                 const std::string& name,
                                 const std::string& full_path,
                                 const VariableMapPtr& config_vars_ptr) {
  auto parsed_optional =
      ParseOptionalArray<T>(obj, name, full_path, config_vars_ptr);
  if (!parsed_optional) {
    throw ParseError(full_path, name, "array");
  }
  return std::move(*parsed_optional);
}

}  // namespace impl
}  // namespace json_config
