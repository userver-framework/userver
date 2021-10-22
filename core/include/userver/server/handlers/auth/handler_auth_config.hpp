#pragma once

#include <string>
#include <vector>

#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

class HandlerAuthConfig final : public yaml_config::YamlConfig {
 public:
  explicit HandlerAuthConfig(yaml_config::YamlConfig value);

  const std::vector<std::string>& GetTypes() const { return types_; };

 private:
  std::vector<std::string> types_;
};

HandlerAuthConfig Parse(const yaml_config::YamlConfig& value,
                        formats::parse::To<HandlerAuthConfig>);

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
