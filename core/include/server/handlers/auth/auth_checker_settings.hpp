#pragma once

#include <boost/optional.hpp>

#include <formats/json/value.hpp>

#include "auth_checker_apikey_settings.hpp"

namespace server {
namespace handlers {
namespace auth {

class AuthCheckerSettings {
 public:
  explicit AuthCheckerSettings(const formats::json::Value& doc);

  const boost::optional<ApiKeysMap>& GetApiKeysMap() const {
    return apikeys_map_;
  }

 private:
  void ParseApikeys(const formats::json::Value& apikeys_map);

  boost::optional<ApiKeysMap> apikeys_map_;
};

}  // namespace auth
}  // namespace handlers
}  // namespace server
