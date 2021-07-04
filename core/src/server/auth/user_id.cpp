#include <userver/server/auth/user_id.hpp>

#include <userver/logging/log.hpp>

namespace server::auth {

std::string ToString(UserId v) { return std::to_string(ToUInt64(v)); }

logging::LogHelper& operator<<(logging::LogHelper& os, UserId v) {
  return os << ToUInt64(v);
}

}  // namespace server::auth
