#pragma once

/// @file userver/etcd/watch_listener.hpp
/// @brief Queue with value change events in etcd

#include <string>

#include <userver/concurrent/queue.hpp>
#include <userver/etcd/key_value_state.hpp>
#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace etcd {

/// @brief Struct that return value change events in etcd
class WatchListener final {
public:
    WatchListener(concurrent::SpscQueue<KeyValueState>::Consumer&& consumer);

    /// @brief Get an event from etcd if there was one, otherwise waits asynchronously until a next event occurs.
    /// @throws EtcdError if event producing coroutine finished or failed
    KeyValueState GetEvent();

private:
    concurrent::SpscQueue<KeyValueState>::Consumer consumer_;
};

}  // namespace etcd

USERVER_NAMESPACE_END
