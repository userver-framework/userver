#pragma once

USERVER_NAMESPACE_BEGIN

namespace redis {

enum class RedisState {
  kInit = 0,
  kInitError,
  kConnected,
  kDisconnecting,
  kDisconnected,
  kDisconnectError
};

}  // namespace redis

USERVER_NAMESPACE_END
