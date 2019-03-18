#include <yaml_config/variable_map.hpp>

#include <formats/yaml/serialize.hpp>

namespace yaml_config {

VariableMap::VariableMap() = default;

VariableMap::VariableMap(formats::yaml::Value yaml) : yaml_(std::move(yaml)) {
  if (!yaml_.IsObject()) {
    throw std::runtime_error("Substitution variable mapping is not an object");
  }
}

bool VariableMap::IsDefined(const std::string& name) const {
  return !yaml_[name].IsMissing();
}

formats::yaml::Value VariableMap::GetVariable(const std::string& name) const {
  auto var = yaml_[name];
  if (var.IsMissing()) {
    throw std::out_of_range("Config variable '" + name + "' is undefined");
  }
  return var;
}

VariableMap VariableMap::ParseFromFile(const std::string& path) {
  formats::yaml::Value config_vars_yaml = formats::yaml::FromFile(path);
  return VariableMap(std::move(config_vars_yaml));
}

}  // namespace yaml_config
