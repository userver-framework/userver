#include "variable_map.hpp"

#include <stdexcept>

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

}  // namespace json_config
