#include <json_config/variable_map.hpp>

#include <fstream>
#include <stdexcept>

#include <formats/json/exception.hpp>
#include <formats/json/serialize.hpp>

namespace json_config {

VariableMap::VariableMap() = default;

VariableMap::VariableMap(formats::json::Value json) : json_(std::move(json)) {
  if (!json_.isObject()) {
    throw std::runtime_error("Substitution variable mapping is not an object");
  }
}

bool VariableMap::IsDefined(const std::string& name) const {
  return !json_[name].isNull();
}

formats::json::Value VariableMap::GetVariable(const std::string& name) const {
  auto var = json_[name];
  if (var.isNull()) {
    throw std::out_of_range("Config variable '" + name + "' is undefined");
  }
  return var;
}

VariableMap VariableMap::ParseFromFile(const std::string& path) {
  std::ifstream input_stream(path);
  formats::json::Value config_vars_json;
  try {
    config_vars_json = formats::json::FromStream(input_stream);
  } catch (const formats::json::JsonException& e) {
    throw std::runtime_error("Cannot parse variables mapping file '" + path +
                             "': " + e.what());
  }

  return VariableMap(std::move(config_vars_json));
}

}  // namespace json_config
