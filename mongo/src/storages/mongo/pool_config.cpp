#include <storages/mongo/pool_config.hpp>

#include <optional>
#include <string>

#include <mongoc/mongoc.h>

#include <formats/parse/to.hpp>
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
constexpr auto kTestMaintenancePeriod = std::chrono::seconds{1};

bool IsValidAppName(const std::string& app_name) {
  return app_name.size() <= MONGOC_HANDSHAKE_APPNAME_MAX &&
         utils::text::IsCString(app_name) && utils::text::IsUtf8(app_name);
}

void CheckDuration(const std::chrono::milliseconds& timeout, const char* name,
                   const std::string& pool_id) {
  auto timeout_ms = timeout.count();
  if (timeout_ms < 0 || timeout_ms > std::numeric_limits<int32_t>::max()) {
    throw InvalidConfigException("Invalid ")
        << name << " in " << pool_id << " pool config: " << timeout_ms << "ms";
  }
}

}  // namespace

static auto Parse(const yaml_config::YamlConfig& config,
                  formats::parse::To<PoolConfig::DriverImpl>) {
  auto impl_str = config.As<std::string>();
  if (impl_str == "mongo-c-driver") {
    return PoolConfig::DriverImpl::kMongoCDriver;
  }
  throw InvalidConfigException("Unknown driver implementation: ") << impl_str;
}

PoolConfig::PoolConfig(const components::ComponentConfig& component_config)
    : conn_timeout(
          component_config["conn_timeout"].As<std::chrono::milliseconds>(
              kDefaultConnTimeout)),
      so_timeout(component_config["so_timeout"].As<std::chrono::milliseconds>(
          kDefaultSoTimeout)),
      queue_timeout(
          component_config["queue_timeout"].As<std::chrono::milliseconds>(
              kDefaultQueueTimeout)),
      initial_size(kDefaultInitialSize),
      max_size(component_config["max_size"].As<size_t>(kDefaultMaxSize)),
      idle_limit(kDefaultIdleLimit),
      connecting_limit(component_config["connecting_limit"].As<size_t>(
          kDefaultConnectingLimit)),
      local_threshold(component_config["local_threshold"]
                          .As<std::optional<std::chrono::milliseconds>>()),
      maintenance_period(
          component_config["maintenance_period"].As<std::chrono::milliseconds>(
              kDefaultMaintenancePeriod)),
      app_name(component_config["appname"].As<std::string>(kDefaultAppName)),
      max_replication_lag(component_config["max_replication_lag"]
                              .As<std::optional<std::chrono::seconds>>()),
      driver_impl(component_config["driver"].As<DriverImpl>(
          DriverImpl::kMongoCDriver)) {
  const auto& pool_id = component_config.Name();
  CheckDuration(conn_timeout, "connection timeout", pool_id);
  CheckDuration(so_timeout, "socket timeout", pool_id);
  CheckDuration(queue_timeout, "queue wait timeout", pool_id);
  if (local_threshold) {
    CheckDuration(*local_threshold, "local threshold", pool_id);
  }
  CheckDuration(maintenance_period, "pool maintenance period", pool_id);

  if (!max_size) {
    throw InvalidConfigException("invalid max pool size in ")
        << pool_id << " pool config";
  }

  auto user_idle_limit =
      component_config["idle_limit"].As<std::optional<size_t>>();
  idle_limit = user_idle_limit ? *user_idle_limit
                               : std::min(kDefaultIdleLimit, max_size);
  if (idle_limit > max_size) {
    throw InvalidConfigException("invalid idle connections limit in ")
        << pool_id << " pool config";
  }

  auto user_initial_size =
      component_config["initial_size"].As<std::optional<size_t>>();
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

PoolConfig::PoolConfig(std::string app_name_, DriverImpl driver_impl_)
    : conn_timeout(kTestConnTimeout),
      so_timeout(kTestSoTimeout),
      queue_timeout(kTestQueueTimeout),
      initial_size(kTestInitialSize),
      max_size(kTestMaxSize),
      idle_limit(kTestIdleLimit),
      connecting_limit(kTestConnectingLimit),
      maintenance_period(kTestMaintenancePeriod),
      app_name(std::move(app_name_)),
      driver_impl(driver_impl_) {
  if (!IsValidAppName(app_name)) {
    throw InvalidConfigException("Invalid appname in test pool config");
  }
}

}  // namespace storages::mongo
