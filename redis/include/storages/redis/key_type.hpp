#pragma once

#include <string>

namespace storages {
namespace redis {

enum class KeyType { kNone, kString, kList, kSet, kZset, kHash, kStream };

KeyType ParseKeyType(const std::string& str);
std::string ToString(KeyType key_type);

}  // namespace redis
}  // namespace storages
