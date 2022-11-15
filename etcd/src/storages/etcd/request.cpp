#include <userver/storages/etcd/request.hpp>

USERVER_NAMESPACE_BEGIN


namespace storages::etcd {

    std::pair<std::string, std::string> Component::KeyValue() {
        return std::make_pair(key_, value_);
    }

    std::string Component::GetKey() {
        return key_;
    }

    std::string Component::GetValue() {
        return value_;
    }


    std::vector<Component> Request::Get() {
        return components_;
    }


}
USERVER_NAMESPACE_END
