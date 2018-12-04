#include <server/handlers/auth/auth_checker_settings.hpp>

#include <stdexcept>

#include <formats/json/serialize.hpp>
#include <logging/log.hpp>

namespace {

const std::string kApikeys = "apikeys";

}  // namespace

namespace server {
namespace handlers {
namespace auth {

AuthCheckerSettings::AuthCheckerSettings(const formats::json::Value& doc) {
  const auto& apikeys_map = doc[kApikeys];
  if (apikeys_map.isNull()) return;
  if (!apikeys_map.isObject())
    throw std::runtime_error("cannot parse " + kApikeys + ", object expected");

  for (auto elem = apikeys_map.begin(); elem != apikeys_map.end(); ++elem) {
    const std::string& apikey_type = elem.GetName();
    if (!elem->isArray())
      throw std::runtime_error("cannot parse " + kApikeys + '.' + apikey_type +
                               ", array expected");
    for (auto key = elem->begin(); key != elem->end(); ++key) {
      if (key->isString()) {
        apikeys_map_[apikey_type].insert(key->asString());
      } else {
        throw std::runtime_error(
            "cannot parse " + kApikeys + '.' + apikey_type + '[' +
            std::to_string(key.GetIndex()) + "], string expected");
      }
    }
  }
}

}  // namespace auth
}  // namespace handlers
}  // namespace server
