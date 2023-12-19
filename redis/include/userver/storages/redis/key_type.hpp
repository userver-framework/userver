#pragma once

#include <string>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

enum class KeyType { kNone, kString, kList, kSet, kZset, kHash, kStream };

KeyType ParseKeyType(std::string_view str);
std::string ToString(KeyType key_type);

}  // namespace storages::redis

USERVER_NAMESPACE_END
