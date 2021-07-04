#include <userver/storages/redis/key_type.hpp>

#include <unordered_map>

namespace std {

template <>
struct hash<storages::redis::KeyType> {
  size_t operator()(storages::redis::KeyType key_type) const {
    return static_cast<size_t>(key_type);
  }
};

}  // namespace std

namespace storages::redis {
namespace {

std::unordered_map<std::string, KeyType> InitKeyTypeMap() {
  return {{"none", KeyType::kNone},    {"string", KeyType::kString},
          {"list", KeyType::kList},    {"set", KeyType::kSet},
          {"zset", KeyType::kZset},    {"hash", KeyType::kHash},
          {"stream", KeyType::kStream}};
}

std::unordered_map<KeyType, std::string> InitKeyTypeNames() {
  auto key_type_map = InitKeyTypeMap();
  std::unordered_map<KeyType, std::string> result;
  for (const auto& elem : key_type_map) {
    result[elem.second] = elem.first;
  }
  return result;
}

}  // namespace

KeyType ParseKeyType(const std::string& str) {
  static const auto types_map = InitKeyTypeMap();
  auto it = types_map.find(str);
  if (it == types_map.end()) {
    throw std::runtime_error("Can't parse KeyType from '" + str + '\'');
  }
  return it->second;
}

std::string ToString(KeyType key_type) {
  static const auto type_names_map = InitKeyTypeNames();
  auto it = type_names_map.find(key_type);
  if (it == type_names_map.end()) {
    throw std::runtime_error("Unknown type: (" +
                             std::to_string(static_cast<int>(key_type)) + ')');
  }
  return it->second;
}

}  // namespace storages::redis
