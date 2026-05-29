#pragma once

/// @file userver/server/handlers/auth/auth_checker_apikey_settings.hpp
/// @brief API key sets and maps for handler authentication

#include <string>
#include <unordered_map>
#include <unordered_set>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

using ApiKeysSet = std::unordered_set<std::string>;
using ApiKeysMap = std::unordered_map<std::string, ApiKeysSet>;

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
