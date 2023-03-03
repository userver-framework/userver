#include <userver/storages/mongo/pool_config.hpp>

#include <optional>
#include <string>

#include <mongoc/mongoc.h>

#include <userver/components/component.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/utils/text.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

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

constexpr utils::TrivialBiMap kDriverImplMapping([](auto selector) {
  return selector().Case(PoolConfig::DriverImpl::kMongoCDriver,
                         "mongo-c-driver");
});

constexpr utils::TrivialBiMap kStatsVerbosityMapping([](auto selector) {
  return selector()
      .Case(StatsVerbosity::kTerse, "terse")
      .Case(StatsVerbosity::kFull, "full");
});

}  // namespace

static auto Parse(const yaml_config::YamlConfig& config,
                  formats::parse::To<PoolConfig::DriverImpl>) {
  return utils::ParseFromValueString<InvalidConfigException>(
      config, kDriverImplMapping);
}

static auto Parse(const yaml_config::YamlConfig& config,
                  formats::parse::To<StatsVerbosity>) {
  return utils::ParseFromValueString<InvalidConfigException>(
      config, kStatsVerbosityMapping);
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
      driver_impl(
          component_config["driver"].As<DriverImpl>(DriverImpl::kMongoCDriver)),
      stats_verbosity(component_config["stats_verbosity"].As<StatsVerbosity>(
          StatsVerbosity::kTerse)) {
  auto user_idle_limit =
      component_config["idle_limit"].As<std::optional<size_t>>();
  idle_limit = user_idle_limit ? *user_idle_limit
                               : std::min(kDefaultIdleLimit, max_size);

  auto user_initial_size =
      component_config["initial_size"].As<std::optional<size_t>>();
  initial_size = user_initial_size ? *user_initial_size
                                   : std::min(kDefaultInitialSize, idle_limit);

  Validate(component_config.Name());
}

PoolConfig::PoolConfig()
    : conn_timeout(kTestConnTimeout),
      so_timeout(kTestSoTimeout),
      queue_timeout(kTestQueueTimeout),
      initial_size(kTestInitialSize),
      max_size(kTestMaxSize),
      idle_limit(kTestIdleLimit),
      connecting_limit(kTestConnectingLimit),
      maintenance_period(kTestMaintenancePeriod),
      app_name(kDefaultAppName),
      driver_impl(DriverImpl::kMongoCDriver),
      stats_verbosity(StatsVerbosity::kTerse) {
  if (!IsValidAppName(app_name)) {
    throw InvalidConfigException("Invalid appname in test pool config");
  }
}

void PoolConfig::Validate(const std::string& pool_id) const {
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

  if (idle_limit > max_size) {
    throw InvalidConfigException("invalid idle connections limit in ")
        << pool_id << " pool config";
  }

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

}  // namespace storages::mongo

USERVER_NAMESPACE_END
