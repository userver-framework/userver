#include <userver/etcd/watch_listener.hpp>

#include <userver/crypto/base64.hpp>
#include <userver/etcd/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace etcd {

WatchListener::WatchListener(concurrent::SpscQueue<KeyValueState>::Consumer&& consumer)
    : consumer_(std::move(consumer)) {}

KeyValueState WatchListener::GetEvent() {
    KeyValueState event;
    if (!consumer_.Pop(event)) {
        throw EtcdError("Consumer pop failed while trying to get etcd key-value event");
    }
    return event;
}

}  // namespace etcd

USERVER_NAMESPACE_END
