#pragma once

#include <formats/json/value.hpp>

#include "auth_checker_apikey_settings.hpp"

namespace server {
namespace handlers {
namespace auth {

class AuthCheckerSettings {
 public:
  explicit AuthCheckerSettings(const formats::json::Value& doc);

  const ApiKeysMap& GetApiKeysMap() const { return apikeys_map_; }

 private:
  ApiKeysMap apikeys_map_;
};

}  // namespace auth
}  // namespace handlers
}  // namespace server
