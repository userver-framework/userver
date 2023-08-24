#include "auth_checker_apikey.hpp"

#include <userver/crypto/algorithm.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/http/http_error.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::apikey {
namespace {

const std::string kApiKeyType = "apikey_type";
const std::string kApiKeyTypeByMethod = "apikey_type_by_method";

const ApiKeysSet& GetApiKeysByType(const AuthCheckerSettings& settings,
                                   const std::string& apikey_type) {
  const auto& apikeys_map = settings.GetApiKeysMap();
  if (!apikeys_map) {
    throw std::runtime_error(
        "apikeys map not found in auth checker settings (possible missing "
        "value in secdist)");
  }

  auto it = apikeys_map->find(apikey_type);
  if (it == apikeys_map->end()) {
    throw std::runtime_error(
        "apikey_type '" + apikey_type +
        "' not found in apikeys settings (possible missing value in secdist)");
  }
  return it->second;
}

bool IsApiKeyAllowed(const std::string& api_key,
                     const ApiKeysSet& allowed_keys) {
  for (const auto& allowed_key : allowed_keys) {
    if (crypto::algorithm::AreStringsEqualConstTime(api_key, allowed_key))
      return true;
  }
  return false;
}

}  // namespace

AuthCheckerApiKey::AuthCheckerApiKey(const HandlerAuthConfig& auth_config,
                                     const AuthCheckerSettings& settings) {
  keys_by_method_.fill(nullptr);
  const auto apikey_type =
      auth_config[kApiKeyType].As<std::optional<std::string>>();
  if (apikey_type) {
    const auto& keys_set = GetApiKeysByType(settings, *apikey_type);
    for (auto method : http::kHandlerMethods)
      keys_by_method_[static_cast<int>(method)] = &keys_set;
  }
  const auto apikey_type_by_method =
      auth_config[kApiKeyTypeByMethod]
          .As<std::optional<ApiKeyTypeByMethodSettings>>();
  if (apikey_type_by_method) {
    for (auto method : http::kHandlerMethods) {
      auto method_idx = static_cast<int>(method);
      auto apikey_type_opt = apikey_type_by_method->apikey_type[method_idx];
      if (apikey_type_opt) {
        const auto& keys_set = GetApiKeysByType(settings, *apikey_type_opt);
        keys_by_method_[method_idx] = &keys_set;
      }
    }
  }
}

[[nodiscard]] AuthCheckResult AuthCheckerApiKey::CheckAuth(
    const http::HttpRequest& request, request::RequestContext&) const {
  const auto* allowed_keys = GetApiKeysForRequest(request);
  if (!allowed_keys) {
    return AuthCheckResult{
        AuthCheckResult::Status::kOk,
        fmt::format("apikey_type is not set for '{}' '{}' requests",
                    request.GetMethodStr(), request.GetRequestPath())};
  }

  const auto& request_apikey =
      request.GetHeader(USERVER_NAMESPACE::http::headers::kApiKey);
  if (request_apikey.empty()) {
    return AuthCheckResult{
        AuthCheckResult::Status::kTokenNotFound,
        "missing or empty " +
            std::string(USERVER_NAMESPACE::http::headers::kApiKey) + " header"};
  }

  if (IsApiKeyAllowed(request_apikey, *allowed_keys)) {
    return AuthCheckResult{AuthCheckResult::Status::kOk,
                           std::string{"IsApiKeyAllowed: OK"}};
  }
  LOG_WARNING() << "access is not allowed with apikey from "
                << USERVER_NAMESPACE::http::headers::kApiKey;

  return AuthCheckResult{
      AuthCheckResult::Status::kForbidden,
      "no valid apikey found in " +
          std::string(USERVER_NAMESPACE::http::headers::kApiKey) + " header"};
}

const ApiKeysSet* AuthCheckerApiKey::GetApiKeysForRequest(
    const http::HttpRequest& request) const {
  auto method_idx = static_cast<size_t>(request.GetMethod());
  if (method_idx >= keys_by_method_.size())
    throw std::runtime_error("method " + request.GetMethodStr() +
                             " is not supported in AuthCheckerApiKey");
  return keys_by_method_[method_idx];
}

AuthCheckerApiKey::ApiKeyTypeByMethodSettings Parse(
    const yaml_config::YamlConfig& value,
    formats::parse::To<AuthCheckerApiKey::ApiKeyTypeByMethodSettings>) {
  AuthCheckerApiKey::ApiKeyTypeByMethodSettings settings;
  for (auto method : http::kHandlerMethods) {
    settings.apikey_type[static_cast<int>(method)] =
        value[http::ToString(method)].As<std::optional<std::string>>();
  }
  return settings;
}

}  // namespace server::handlers::auth::apikey

USERVER_NAMESPACE_END
