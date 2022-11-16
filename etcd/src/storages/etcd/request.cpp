#include <userver/storages/etcd/request.hpp>

USERVER_NAMESPACE_BEGIN


namespace storages::etcd {

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


}
USERVER_NAMESPACE_END
