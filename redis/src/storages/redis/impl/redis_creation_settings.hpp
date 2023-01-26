#pragma once

#include <unordered_map>

#include <userver/storages/redis/impl/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

struct RedisCreationSettings {
  ConnectionSecurity connection_security = ConnectionSecurity::kNone;
  bool send_readonly{false};
};

}  // namespace redis

USERVER_NAMESPACE_END
