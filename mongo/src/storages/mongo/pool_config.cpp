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

PoolConfig Parse(const yaml_config::YamlConfig& config,
                 formats::parse::To<PoolConfig>) {
  PoolConfig result{};
  result.conn_timeout =
      config["conn_timeout"].As<std::chrono::milliseconds>(result.conn_timeout);
  result.so_timeout =
      config["so_timeout"].As<std::chrono::milliseconds>(result.so_timeout);
  result.queue_timeout = config["queue_timeout"].As<std::chrono::milliseconds>(
      result.queue_timeout);
  result.max_size = config["max_size"].As<size_t>(result.max_size);
  result.connecting_limit =
      config["connecting_limit"].As<size_t>(result.connecting_limit);
  result.local_threshold =
      config["local_threshold"].As<std::optional<std::chrono::milliseconds>>();
  result.maintenance_period =
      config["maintenance_period"].As<std::chrono::milliseconds>(
          result.maintenance_period);
  result.app_name = config["appname"].As<std::string>(result.app_name);
  result.max_replication_lag =
      config["max_replication_lag"].As<std::optional<std::chrono::seconds>>();
  result.driver_impl =
      config["driver"].As<PoolConfig::DriverImpl>(result.driver_impl);
  result.stats_verbosity =
      config["stats_verbosity"].As<StatsVerbosity>(result.stats_verbosity);
  result.cc_config =
      config["congestion_control"]
          .As<congestion_control::v2::LinearController::StaticConfig>();

  auto user_idle_limit = config["idle_limit"].As<std::optional<size_t>>();
  result.idle_limit = user_idle_limit
                          ? *user_idle_limit
                          : std::min(result.idle_limit, result.max_size);

  auto user_initial_size = config["initial_size"].As<std::optional<size_t>>();
  result.initial_size = user_initial_size
                            ? *user_initial_size
                            : std::min(result.initial_size, result.idle_limit);

  return result;
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
