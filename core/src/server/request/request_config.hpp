#pragma once

#include <string>

#include <yaml_config/yaml_config.hpp>

namespace server {
namespace request {

class RequestConfig : public yaml_config::YamlConfig {
 public:
  enum class Type { kHttp };

  RequestConfig(formats::yaml::Node yaml, std::string full_path,
                yaml_config::VariableMapPtr config_vars_ptr);

  const Type& GetType() const;

  static RequestConfig ParseFromYaml(
      const formats::yaml::Node& yaml, const std::string& full_path,
      const yaml_config::VariableMapPtr& config_vars_ptr);

  static const std::string& TypeToString(Type type);

 private:
  Type type_ = Type::kHttp;
};

}  // namespace request
}  // namespace server
