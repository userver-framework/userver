#include <testsuite/testsuite_support.hpp>

#include <utils/periodic_task.hpp>

namespace components {

namespace {

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
      config["testsuite-pg-execute-timeout"].As<std::chrono::milliseconds>(0),
      config["testsuite-pg-statement-timeout"].As<std::chrono::milliseconds>(0),
      config["testsuite-pg-readonly-master-expected"].As<bool>(false)
          ? testsuite::PostgresControl::ReadonlyMaster::kExpected
          : testsuite::PostgresControl::ReadonlyMaster::kNotExpected);
}

testsuite::RedisControl ParseRedisControl(
    const components::ComponentConfig& config) {
  return testsuite::RedisControl{
      config["testsuite-redis-timeout-connect"].As<std::chrono::milliseconds>(
          0),
      config["testsuite-redis-timeout-single"].As<std::chrono::milliseconds>(0),
      config["testsuite-redis-timeout-all"].As<std::chrono::milliseconds>(0),
  };
}

}  // namespace

TestsuiteSupport::TestsuiteSupport(const components::ComponentConfig& config,
                                   const components::ComponentContext& context)
    : cache_control_(
          ParsePeriodicUpdatesMode(config["testsuite-periodic-update-enabled"]
                                       .As<std::optional<bool>>())),
      postgres_control_(ParsePostgresControl(config)),
      redis_control_(ParseRedisControl(config)),
      metrics_storage_(context.FindComponent<components::StatisticsStorage>()
                           .GetMetricsStorage()) {}

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

void TestsuiteSupport::ResetMetrics() { metrics_storage_->ResetMetrics(); }

}  // namespace components
