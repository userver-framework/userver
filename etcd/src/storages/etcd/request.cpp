#include <userver/storages/etcd/request.hpp>

USERVER_NAMESPACE_BEGIN


namespace storages::etcd {

    Component::Component(const std::string& key, const std::string& value) :
        key_(key), value_(value)
    {}

    std::pair<std::string, std::string> Component::KeyValue() const {
        return std::make_pair(key_, value_);
    }

    std::string Component::GetKey() const {
        return key_;
    }

    std::string Component::GetValue() const {
        return value_;
    }


    std::vector<Component> Request::Get() const {
        return components_;
    }

    Request::Request(const std::vector<Component>& components) :
        components_(components)
    {}


}  // namespace storages::etcd
USERVER_NAMESPACE_END
