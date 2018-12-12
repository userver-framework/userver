#pragma once

#include <memory>
#include <string>

#include <formats/yaml.hpp>

namespace yaml_config {

class VariableMap {
 public:
  VariableMap();
  explicit VariableMap(formats::yaml::Node yaml);

  const formats::yaml::Node& Yaml() const { return yaml_; }

  bool IsDefined(const std::string& name) const;
  formats::yaml::Node GetVariable(const std::string& name) const;

  static VariableMap ParseFromFile(const std::string& path);

 private:
  formats::yaml::Node yaml_;
};

using VariableMapPtr = std::shared_ptr<VariableMap>;

}  // namespace yaml_config
