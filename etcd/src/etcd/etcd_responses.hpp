#pragma once

#include <userver/formats/parse/common_containers.hpp>

#include <schemas/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace etcd {

struct EtcdWatchResponse final {
    std::vector<etcd_schemas::RawKeyValueState> raw_key_value_states;
};

}  // namespace etcd

namespace formats::parse {

etcd::EtcdWatchResponse Parse(const formats::json::Value& value, To<etcd::EtcdWatchResponse>);

}  // namespace formats::parse

USERVER_NAMESPACE_END
