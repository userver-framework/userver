#pragma once

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
