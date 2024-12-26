#pragma once

#include <unordered_map>

#include <userver/storages/redis/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

struct RedisCreationSettings {
    ConnectionSecurity connection_security = ConnectionSecurity::kNone;
    bool send_readonly{false};
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
