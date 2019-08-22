#include <storages/redis/client.hpp>

#include <redis/sentinel.hpp>

namespace storages {
namespace redis {

std::string CreateTmpKey(const std::string& key, std::string prefix) {
  return ::redis::Sentinel::CreateTmpKey(key, std::move(prefix));
}

}  // namespace redis
}  // namespace storages
