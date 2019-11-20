#pragma once

#include <string>

namespace server::auth {

// Possible values of 'X-Ya-User-Ticket-Provider' header.
enum class UserProvider : int {
  kYandex,
  kYandexTeam,
};

std::string ToString(UserProvider provider);

}  // namespace server::auth
