#include <userver/storages/redis/key_type.hpp>

#include <stdexcept>

#include <fmt/format.h>

#include <userver/utils/trivial_map.hpp>

namespace std {

template <>
struct hash<USERVER_NAMESPACE::storages::redis::KeyType> {
  size_t operator()(
      USERVER_NAMESPACE::storages::redis::KeyType key_type) const {
    return static_cast<size_t>(key_type);
  }
};

}  // namespace std

USERVER_NAMESPACE_BEGIN

namespace storages::redis {
namespace {

constexpr utils::TrivialBiMap kKeyTypeMap = [](auto selector) {
  return selector()
      .Case("none", KeyType::kNone)
      .Case("string", KeyType::kString)
      .Case("list", KeyType::kList)
      .Case("set", KeyType::kSet)
      .Case("zset", KeyType::kZset)
      .Case("hash", KeyType::kHash)
      .Case("stream", KeyType::kStream);
};

}  // namespace

KeyType ParseKeyType(std::string_view str) {
  auto value = kKeyTypeMap.TryFind(str);
  if (!value) {
    throw std::runtime_error(fmt::format("Can't parse KeyType from '{}'", str));
  }
  return *value;
}

std::string ToString(KeyType key_type) {
  auto value = kKeyTypeMap.TryFind(key_type);
  if (!value) {
    throw std::runtime_error("Unknown type: (" +
                             std::to_string(static_cast<int>(key_type)) + ')');
  }
  return std::string{*value};
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
