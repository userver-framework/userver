#include <yaml_config/variable_map.hpp>

#include <fstream>
#include <stdexcept>

namespace yaml_config {

VariableMap::VariableMap() = default;

VariableMap::VariableMap(formats::yaml::Node yaml) : yaml_(std::move(yaml)) {
  if (!yaml_.IsMap()) {
    throw std::runtime_error("Substitution variable mapping is not an object");
  }
}

bool VariableMap::IsDefined(const std::string& name) const {
  return yaml_[name].IsDefined();
}

formats::yaml::Node VariableMap::GetVariable(const std::string& name) const {
  auto var = yaml_[name];
  if (!var) {
    throw std::out_of_range("Config variable '" + name + "' is undefined");
  }
  return var;
}

VariableMap VariableMap::ParseFromFile(const std::string& path) {
  std::ifstream input_stream(path);
  formats::yaml::Node config_vars_yaml;
  try {
    config_vars_yaml = formats::yaml::Load(input_stream);
  } catch (const formats::yaml::Exception& e) {
    throw std::runtime_error("Cannot parse variables mapping file '" + path +
                             "': " + e.what());
  }

  return VariableMap(std::move(config_vars_yaml));
}

}  // namespace yaml_config
