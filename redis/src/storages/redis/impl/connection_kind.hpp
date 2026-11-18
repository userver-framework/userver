#pragma once

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

enum class ConnectionKind { kCommand, kSubscription };

enum class ConnectionMode { kCommands, kSubscriptions, kCommandsAndSubscriptions };

constexpr bool HasCommands(ConnectionMode mode) {
    return mode == ConnectionMode::kCommands || mode == ConnectionMode::kCommandsAndSubscriptions;
}

constexpr bool HasSubscriptions(ConnectionMode mode) {
    return mode == ConnectionMode::kSubscriptions || mode == ConnectionMode::kCommandsAndSubscriptions;
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
