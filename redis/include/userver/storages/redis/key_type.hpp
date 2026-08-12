#pragma once

/// @file
/// @brief @copybrief storages::redis::KeyType

#include <string>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

/// @brief Type of the Redis value stored by a key.
///
/// Returned by storages::redis::Client and storages::redis::Transaction from membed function `Type()`
enum class KeyType { kNone, kString, kList, kSet, kZset, kHash, kStream };

KeyType ParseKeyType(std::string_view str);
std::string ToString(KeyType key_type);

}  // namespace storages::redis

USERVER_NAMESPACE_END
