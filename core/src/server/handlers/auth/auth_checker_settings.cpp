#include <userver/server/handlers/auth/auth_checker_settings.hpp>

#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/string_literal.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr utils::StringLiteral kApikeys = "apikeys";

}  // namespace

namespace server::handlers::auth {

AuthCheckerSettings::AuthCheckerSettings(const formats::json::Value& doc) {
    if (doc.HasMember(kApikeys)) {
        ParseApikeys(doc[kApikeys]);
    }
}

void AuthCheckerSettings::ParseApikeys(const formats::json::Value& apikeys_map) {
    apikeys_map_ = apikeys_map.As<ApiKeysMap>();
}

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
