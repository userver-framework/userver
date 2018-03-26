#include "variable_map.hpp"

#include <fstream>
#include <stdexcept>

#include <json/reader.h>

namespace json_config {

VariableMap::VariableMap() = default;

VariableMap::VariableMap(Json::Value json) : json_(std::move(json)) {
  if (!json_.isObject()) {
    throw std::runtime_error("Substitution variable mapping is not an object");
  }
}

bool VariableMap::IsDefined(const std::string& name) const {
  return !json_[name].isNull();
}

const Json::Value& VariableMap::GetVariable(const std::string& name) const {
  const auto& var = json_[name];
  if (var.isNull()) {
    throw std::out_of_range("Config variable '" + name + "' is undefined");
  }
  return var;
}

VariableMap VariableMap::ParseFromFile(const std::string& path) {
  std::ifstream input_stream(path);
  if (!input_stream) {
    throw std::runtime_error("Cannot open variables mapping file '" + path +
                             '\'');
  }

  Json::Reader reader;
  Json::Value config_vars_json;
  if (!reader.parse(input_stream, config_vars_json)) {
    throw std::runtime_error("Cannot parse variables mapping file '" + path +
                             "': " + reader.getFormattedErrorMessages());
  }

  return VariableMap(std::move(config_vars_json));
}

}  // namespace json_config
