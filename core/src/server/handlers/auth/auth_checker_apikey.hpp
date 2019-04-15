#pragma once

#include <server/handlers/auth/auth_checker_base.hpp>
#include <server/handlers/auth/auth_checker_factory.hpp>
#include <server/handlers/auth/auth_checker_settings.hpp>
#include <server/handlers/auth/handler_auth_config.hpp>
#include <server/http/handler_methods.hpp>

namespace server {
namespace handlers {
namespace auth {

class AuthCheckerApiKey : public AuthCheckerBase {
 public:
  AuthCheckerApiKey(const HandlerAuthConfig& auth_config,
                    const AuthCheckerSettings& settings);

  [[nodiscard]] AuthCheckResult CheckAuth(
      const http::HttpRequest& request,
      request::RequestContext& context) const override;

 private:
  struct ApiKeyTypeByMethodSettings {
    std::array<boost::optional<std::string>, http::kHandlerMethodsMax + 1>
        apikey_type;

    static ApiKeyTypeByMethodSettings ParseFromYaml(
        const formats::yaml::Value& yaml, const std::string& full_path,
        const yaml_config::VariableMapPtr& config_vars_ptr);
  };

  const ApiKeysSet* GetApiKeysForRequest(
      const http::HttpRequest& request) const;
  static const ApiKeysSet& GetApiKeysByType(const AuthCheckerSettings& settings,
                                            const std::string& apikey_type);
  bool IsApiKeyAllowed(const std::string& api_key,
                       const ApiKeysSet& allowed_keys) const;

  std::array<const ApiKeysSet*, http::kHandlerMethodsMax + 1> keys_by_method_{};
};

}  // namespace auth
}  // namespace handlers
}  // namespace server
