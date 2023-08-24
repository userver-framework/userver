#pragma once

#include <optional>

#include <userver/formats/json/value.hpp>

#include "auth_checker_apikey_settings.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

class AuthCheckerSettings final {
 public:
  explicit AuthCheckerSettings(const formats::json::Value& doc);

  const std::optional<ApiKeysMap>& GetApiKeysMap() const {
    return apikeys_map_;
  }

 private:
  void ParseApikeys(const formats::json::Value& apikeys_map);

  std::optional<ApiKeysMap> apikeys_map_;
};

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
