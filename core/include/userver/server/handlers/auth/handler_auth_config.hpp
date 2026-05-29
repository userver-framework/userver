#pragma once

/// @file userver/server/handlers/auth/handler_auth_config.hpp
/// @brief @copybrief server::handlers::auth::HandlerAuthConfig

#include <string>
#include <vector>

#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

/// @brief Handler authentication configuration
class HandlerAuthConfig final : public yaml_config::YamlConfig {
public:
    explicit HandlerAuthConfig(yaml_config::YamlConfig value);

    const std::vector<std::string>& GetTypes() const { return types_; }

private:
    std::vector<std::string> types_;
};

HandlerAuthConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<HandlerAuthConfig>);

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
