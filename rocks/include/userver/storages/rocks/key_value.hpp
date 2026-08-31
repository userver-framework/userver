#pragma once

/// @file userver/storages/rocks/key_value.hpp
/// @brief @copybrief storages::rocks::KeyValue

#include <string>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

/// @brief A key-value pair returned by scan operations.
struct KeyValue {
    std::string key;
    std::string value;
};

}  // namespace storages::rocks

USERVER_NAMESPACE_END
