#include <userver/storages/etcd/key_value.hpp>

#include <etcd/api/mvccpb/kv.pb.h>

USERVER_NAMESPACE_BEGIN


namespace storages::etcd {

    KeyValue::KeyValue() = default;
    KeyValue::KeyValue(KeyValue&&) = default;
    KeyValue::KeyValue(const KeyValueNative& key_value) : key_value_(key_value) {}
    KeyValue& KeyValue::operator=(KeyValue&&) = default;
    KeyValue::~KeyValue() = default;

    std::pair<const std::string&, const std::string&> KeyValue::Pair() {
        return {GetKey(), GetValue()};
    }

    const std::string& KeyValue::GetKey() const {
        return key_value_->key();
    }

    const std::string& KeyValue::GetValue() const {
        return key_value_->value();
    }
}

USERVER_NAMESPACE_END