#pragma once

/// @file userver/server/auth/user_provider.hpp
/// @brief @copybrief server::auth::UserProvider

#include <string>

USERVER_NAMESPACE_BEGIN

namespace server::auth {

/// @brief Source of the user ticket
enum class UserProvider : int {
    kYandex,
    kYandexTeam,
};

std::string ToString(UserProvider provider);

}  // namespace server::auth

USERVER_NAMESPACE_END
