#pragma once

#include <string>

USERVER_NAMESPACE_BEGIN

namespace storages {
namespace redis {

enum class KeyType { kNone, kString, kList, kSet, kZset, kHash, kStream };

KeyType ParseKeyType(const std::string& str);
std::string ToString(KeyType key_type);

}  // namespace redis
}  // namespace storages

USERVER_NAMESPACE_END
