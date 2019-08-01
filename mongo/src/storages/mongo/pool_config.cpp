#include <storages/mongo/pool_config.hpp>

#include <string>

#include <mongoc/mongoc.h>

#include <boost/optional.hpp>

#include <storages/mongo/exception.hpp>
#include <utils/text.hpp>

namespace storages::mongo {
namespace {

const std::string kDefaultAppName = "userver";

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

void CheckTimeout(const std::chrono::milliseconds& timeout, const char* name,
                  const std::string& pool_id) {
  auto timeout_ms = timeout.count();
  if (timeout_ms < 0 || timeout_ms > std::numeric_limits<int32_t>::max()) {
    throw InvalidConfigException("Invalid ")
        << name << " in " << pool_id << " pool config: " << timeout_ms << "ms";
  }
}

}  // namespace

PoolConfig::PoolConfig(const components::ComponentConfig& component_config)
    : conn_timeout(
          component_config.ParseDuration("conn_timeout", kDefaultConnTimeout)),
      so_timeout(
          component_config.ParseDuration("so_timeout", kDefaultSoTimeout)),
      queue_timeout(component_config.ParseDuration("queue_timeout",
                                                   kDefaultQueueTimeout)),
      initial_size(kDefaultInitialSize),
      max_size(component_config.Parse<size_t>("max_size", kDefaultMaxSize)),
      idle_limit(kDefaultIdleLimit),
      connecting_limit(component_config.Parse<size_t>("connecting_limit",
                                                      kDefaultConnectingLimit)),
      app_name(
          component_config.Parse<std::string>("appname", kDefaultAppName)) {
  const auto& pool_id = component_config.Name();
  CheckTimeout(conn_timeout, "connection timeout", pool_id);
  CheckTimeout(so_timeout, "socket timeout", pool_id);
  CheckTimeout(queue_timeout, "queue wait timeout", pool_id);

  if (!max_size) {
    throw InvalidConfigException("invalid max pool size in ")
        << pool_id << " pool config";
  }

  auto user_idle_limit =
      component_config.Parse<boost::optional<size_t>>("idle_limit");
  idle_limit = user_idle_limit ? *user_idle_limit
                               : std::min(kDefaultIdleLimit, max_size);
  if (idle_limit > max_size) {
    throw InvalidConfigException("invalid idle connections limit in ")
        << pool_id << " pool config";
  }

  auto user_initial_size =
      component_config.Parse<boost::optional<size_t>>("initial_size");
  initial_size = user_initial_size ? *user_initial_size
                                   : std::min(kDefaultInitialSize, idle_limit);
  if (initial_size > idle_limit) {
    throw InvalidConfigException("invalid initial connections count in ")
        << pool_id << " pool config";
  }

  if (!connecting_limit) {
    throw InvalidConfigException("invalid establishing connections limit in ")
        << pool_id << " pool config";
  }

  if (!IsValidAppName(app_name)) {
    throw InvalidConfigException("Invalid appname in ")
        << pool_id << " pool config";
  }
}

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

}  // namespace storages::mongo
