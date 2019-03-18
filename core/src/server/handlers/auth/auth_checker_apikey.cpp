#include "auth_checker_apikey.hpp"

#include <crypto/algorithm.hpp>
#include <server/http/http_error.hpp>
#include <yaml_config/value.hpp>

namespace {

const std::string kApiKeyHeader = "X-YaTaxi-API-Key";
const std::string kApiKeyType = "apikey_type";
const std::string kApiKeyTypeByMethod = "apikey_type_by_method";

}  // namespace

namespace server {
namespace handlers {
namespace auth {

AuthCheckerApiKey::AuthCheckerApiKey(const HandlerAuthConfig& auth_config,
                                     const AuthCheckerSettings& settings) {
  keys_by_method_.fill(nullptr);
  auto apikey_type =
      auth_config.Parse<boost::optional<std::string>>(kApiKeyType);
  if (apikey_type) {
    const auto& keys_set = GetApiKeysByType(settings, *apikey_type);
    for (auto method : http::kHandlerMethods)
      keys_by_method_[static_cast<int>(method)] = &keys_set;
  }
  auto apikey_type_by_method =
      auth_config.Parse<boost::optional<ApiKeyTypeByMethodSettings>>(
          kApiKeyTypeByMethod);
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

void AuthCheckerApiKey::CheckAuth(const http::HttpRequest& request) const {
  const auto* allowed_keys = GetApiKeysForRequest(request);
  if (!allowed_keys) {
    LOG_DEBUG() << "CheckAuth: OK, apikey_type is not set for '" +
                       request.GetMethodStr() + ' ' + request.GetRequestPath() +
                       "' requests";
    return;
  }

  const auto& request_apikey = request.GetHeader(kApiKeyHeader);
  if (request_apikey.empty())
    throw http::BadRequest("missing or empty " + kApiKeyHeader + " header");

  if (IsApiKeyAllowed(request_apikey, *allowed_keys)) {
    LOG_DEBUG() << "IsApiKeyAllowed: OK";
    return;
  } else {
    LOG_WARNING() << "invalid apikey in " << kApiKeyHeader;
  }
  throw http::HttpException(
      http::HttpStatus::kForbidden,
      "no valid apikey found in " + kApiKeyHeader + " header");
}

const ApiKeysSet* AuthCheckerApiKey::GetApiKeysForRequest(
    const http::HttpRequest& request) const {
  auto method_idx = static_cast<size_t>(request.GetMethod());
  if (method_idx >= keys_by_method_.size())
    throw std::runtime_error("method " + request.GetMethodStr() +
                             " is not supported in AuthCheckerApiKey");
  return keys_by_method_[method_idx];
}

const ApiKeysSet& AuthCheckerApiKey::GetApiKeysByType(
    const AuthCheckerSettings& settings, const std::string& apikey_type) {
  const auto& apikeys_map = settings.GetApiKeysMap();

  auto it = apikeys_map.find(apikey_type);
  if (it == apikeys_map.end()) {
    throw std::runtime_error(
        "apikey_type '" + apikey_type +
        "' not found in apikeys settings (possible missing value in secdist)");
  }
  return it->second;
}

bool AuthCheckerApiKey::IsApiKeyAllowed(const std::string& api_key,
                                        const ApiKeysSet& allowed_keys) const {
  for (const auto& allowed_key : allowed_keys) {
    if (crypto::algorithm::AreStringsEqualConstTime(api_key, allowed_key))
      return true;
  }
  return false;
}

AuthCheckerApiKey::ApiKeyTypeByMethodSettings
AuthCheckerApiKey::ApiKeyTypeByMethodSettings::ParseFromYaml(
    const formats::yaml::Value& yaml, const std::string& full_path,
    const yaml_config::VariableMapPtr& config_vars_ptr) {
  ApiKeyTypeByMethodSettings settings;
  for (auto method : http::kHandlerMethods) {
    settings.apikey_type[static_cast<int>(method)] =
        yaml_config::Parse<boost::optional<std::string>>(
            yaml, http::ToString(method), full_path, config_vars_ptr);
  }
  return settings;
}

}  // namespace auth
}  // namespace handlers
}  // namespace server
