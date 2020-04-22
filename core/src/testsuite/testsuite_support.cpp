#include <testsuite/testsuite_support.hpp>

#include <utils/periodic_task.hpp>

namespace components {

namespace {

const std::string kPeriodicUpdateEnabled = "testsuite-periodic-update-enabled";
const std::string kPostgresExecuteTimeout = "testsuite-pg-execute-timeout";
const std::string kPostgresStatementTimeout = "testsuite-pg-statement-timeout";

const std::string kRedisConnectTimeout = "testsuite-redis-timeout-connect";
const std::string kRedisTimeoutSingle = "testsuite-redis-timeout-single";
const std::string kRedisTimeoutAll = "testsuite-redis-timeout-all";

testsuite::CacheControl::PeriodicUpdatesMode ParsePeriodicUpdatesMode(
    const std::optional<bool>& config_value) {
  using PeriodicUpdatesMode = testsuite::CacheControl::PeriodicUpdatesMode;
  if (!config_value) return PeriodicUpdatesMode::kDefault;
  return *config_value ? PeriodicUpdatesMode::kEnabled
                       : PeriodicUpdatesMode::kDisabled;
}

testsuite::PostgresControl ParsePostgresControl(
    const components::ComponentConfig& config) {
  return testsuite::PostgresControl(
      config.ParseDuration(kPostgresExecuteTimeout,
                           std::chrono::milliseconds::zero()),
      config.ParseDuration(kPostgresStatementTimeout,
                           std::chrono::milliseconds::zero()));
}

testsuite::RedisControl ParseRedisControl(
    const components::ComponentConfig& config) {
  return testsuite::RedisControl{
      config.ParseDuration(kRedisConnectTimeout,
                           std::chrono::milliseconds::zero()),
      config.ParseDuration(kRedisTimeoutSingle,
                           std::chrono::milliseconds::zero()),
      config.ParseDuration(kRedisTimeoutAll, std::chrono::milliseconds::zero()),
  };
}

}  // namespace

TestsuiteSupport::TestsuiteSupport(const components::ComponentConfig& config,
                                   const components::ComponentContext&)
    : cache_control_(ParsePeriodicUpdatesMode(
          config.ParseOptionalBool(kPeriodicUpdateEnabled))),
      postgres_control_(ParsePostgresControl(config)),
      redis_control_(ParseRedisControl(config)) {}

testsuite::CacheControl& TestsuiteSupport::GetCacheControl() {
  return cache_control_;
}

testsuite::ComponentControl& TestsuiteSupport::GetComponentControl() {
  return component_control_;
}

testsuite::PeriodicTaskControl& TestsuiteSupport::GetPeriodicTaskControl() {
  return periodic_task_control_;
}

const testsuite::PostgresControl& TestsuiteSupport::GetPostgresControl() {
  return postgres_control_;
}

const testsuite::RedisControl& TestsuiteSupport::GetRedisControl() {
  return redis_control_;
}

void TestsuiteSupport::InvalidateEverything(cache::UpdateType update_type) {
  std::lock_guard lock(invalidation_mutex_);
  cache_control_.InvalidateAllCaches(update_type);
  component_control_.InvalidateComponents();
}

void TestsuiteSupport::InvalidateCaches(cache::UpdateType update_type,
                                        const std::vector<std::string>& names) {
  std::lock_guard lock(invalidation_mutex_);
  cache_control_.InvalidateCaches(update_type, names);
}

}  // namespace components
