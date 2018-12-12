#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace server {
namespace handlers {
namespace auth {

using ApiKeysSet = std::unordered_set<std::string>;
using ApiKeysMap = std::unordered_map<std::string, ApiKeysSet>;

}  // namespace auth
}  // namespace handlers
}  // namespace server
