#include <userver/server/auth/user_provider.hpp>

#include <stdexcept>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::auth {

std::string ToString(UserProvider provider) {
  switch (provider) {
    case UserProvider::kYandex:
      return "yandex";
    case UserProvider::kYandexTeam:
      return "yandex_team";
  }

  UASSERT_MSG(false, "Unknown user provider");
  throw std::runtime_error("Unknown user provider " +
                           std::to_string(static_cast<int>(provider)));
}

}  // namespace server::auth

USERVER_NAMESPACE_END
