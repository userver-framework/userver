#pragma once

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

/// Represents the state of redis instance connection
enum class RedisState {
    /// Initializing context and establishing connection
    kInit = 0,

    /// Connection initialization failed
    kInitError,

    /// Connection established and ready to send commands
    kConnected,

    /// Closing connection, all remaining commands are dropped
    kDisconnecting,

    /// Connection successfully closed
    kDisconnected,

    /// An error occurred while closing connection
    kDisconnectError
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
