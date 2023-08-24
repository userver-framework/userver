#pragma once

#include <array>
#include <optional>

#include <userver/yaml_config/yaml_config.hpp>

#include <server/http/handler_methods.hpp>
#include <userver/server/handlers/auth/auth_checker_base.hpp>
#include <userver/server/handlers/auth/auth_checker_factory.hpp>
#include <userver/server/handlers/auth/auth_checker_settings.hpp>
#include <userver/server/handlers/auth/handler_auth_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::apikey {

class AuthCheckerApiKey : public AuthCheckerBase {
 public:
  AuthCheckerApiKey(const HandlerAuthConfig& auth_config,
                    const AuthCheckerSettings& settings);

  [[nodiscard]] AuthCheckResult CheckAuth(
      const http::HttpRequest& request,
      request::RequestContext& context) const override;

  [[nodiscard]] bool SupportsUserAuth() const noexcept override {
    return false;
  }

 private:
  struct ApiKeyTypeByMethodSettings {
    std::array<std::optional<std::string>, http::kHandlerMethodsMax + 1>
        apikey_type;
  };

  friend ApiKeyTypeByMethodSettings Parse(
      const yaml_config::YamlConfig& value,
      formats::parse::To<ApiKeyTypeByMethodSettings>);

  const ApiKeysSet* GetApiKeysForRequest(
      const http::HttpRequest& request) const;

  std::array<const ApiKeysSet*, http::kHandlerMethodsMax + 1> keys_by_method_{};
};

}  // namespace server::handlers::auth::apikey

USERVER_NAMESPACE_END
