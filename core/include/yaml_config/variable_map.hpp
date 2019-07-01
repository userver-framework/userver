#pragma once

#include <memory>
#include <string>

#include <formats/yaml.hpp>

namespace yaml_config {

class VariableMap final {
 public:
  VariableMap();
  explicit VariableMap(formats::yaml::Value yaml);

  const formats::yaml::Value& Yaml() const { return yaml_; }

  bool IsDefined(const std::string& name) const;
  formats::yaml::Value GetVariable(const std::string& name) const;

  static VariableMap ParseFromFile(const std::string& path);

 private:
  formats::yaml::Value yaml_;
};

using VariableMapPtr = std::shared_ptr<VariableMap>;

}  // namespace yaml_config
