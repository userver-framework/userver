#include <storages/mongo_ng/pool_config.hpp>

#include <mongoc/mongoc.h>

#include <storages/mongo_ng/exception.hpp>
#include <utils/text.hpp>

namespace storages::mongo_ng {
namespace {

constexpr auto kTestConnTimeout = std::chrono::seconds{5};
constexpr auto kTestSoTimeout = std::chrono::seconds{5};
constexpr auto kTestQueueTimeout = std::chrono::milliseconds{10};
constexpr size_t kTestInitialSize = 1;
constexpr size_t kTestMaxSize = 16;
constexpr size_t kTestIdleLimit = 4;
constexpr size_t kTestConnectingLimit = 8;

bool IsValidAppName(const std::string& app_name) {
  return app_name.size() <= MONGOC_HANDSHAKE_APPNAME_MAX &&
         utils::text::IsCString(app_name) && utils::text::IsUtf8(app_name);
}

}  // namespace

PoolConfig::PoolConfig(std::string app_name_)
    : conn_timeout(kTestConnTimeout),
      so_timeout(kTestSoTimeout),
      queue_timeout(kTestQueueTimeout),
      initial_size(kTestInitialSize),
      max_size(kTestMaxSize),
      idle_limit(kTestIdleLimit),
      connecting_limit(kTestConnectingLimit),
      app_name(std::move(app_name_)) {
  if (!IsValidAppName(app_name)) {
    throw InvalidConfigException("Invalid appname in test pool config");
  }
}

}  // namespace storages::mongo_ng
