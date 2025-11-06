#pragma once

/// @file userver/etcd/key_value_state.hpp
/// @brief @copybrief etcd::KeyValueState

#include <string>

USERVER_NAMESPACE_BEGIN

namespace etcd {

/// @brief Struct with key value pair from etcd. It represents current status of key value pair.
struct KeyValueState final {
    std::string key;
    std::string value;
    std::int32_t version;
};

}  // namespace etcd

USERVER_NAMESPACE_END
