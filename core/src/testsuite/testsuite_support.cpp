#include <userver/testsuite/testsuite_support.hpp>

#include <userver/components/component.hpp>
#include <userver/testsuite/tasks.hpp>
#include <userver/utils/periodic_task.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

testsuite::CacheControl::PeriodicUpdatesMode ParsePeriodicUpdatesMode(
    const std::optional<bool>& config_value) {
  using PeriodicUpdatesMode = testsuite::CacheControl::PeriodicUpdatesMode;
  if (!config_value) return PeriodicUpdatesMode::kDefault;
  return *config_value ? PeriodicUpdatesMode::kEnabled
                       : PeriodicUpdatesMode::kDisabled;
}

testsuite::DumpControl ParseDumpControl(
    const components::ComponentConfig& config) {
  using PeriodicsMode = testsuite::DumpControl::PeriodicsMode;
  const auto periodics_mode =
      config["testsuite-periodic-dumps-enabled"].As<bool>(true)
          ? PeriodicsMode::kEnabled
          : PeriodicsMode::kDisabled;
  return testsuite::DumpControl{periodics_mode};
}

testsuite::PostgresControl ParsePostgresControl(
    const components::ComponentConfig& config) {
  return {
      config["testsuite-pg-execute-timeout"].As<std::chrono::milliseconds>(0),
      config["testsuite-pg-statement-timeout"].As<std::chrono::milliseconds>(0),
      config["testsuite-pg-readonly-master-expected"].As<bool>(false)
          ? testsuite::PostgresControl::ReadonlyMaster::kExpected
          : testsuite::PostgresControl::ReadonlyMaster::kNotExpected};
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

std::unique_ptr<testsuite::TestsuiteTasks> ParseTestsuiteTasks(
    const components::ComponentConfig& config) {
  const bool is_enabled = config["testsuite-tasks-enabled"].As<bool>(false);
  return std::make_unique<testsuite::TestsuiteTasks>(is_enabled);
}

testsuite::GrpcControl ParseGrpcControl(
    const components::ComponentConfig& config) {
  bool is_tls_enabled{config["testsuite-grpc-is-tls-enabled"].As<bool>(false)};
  std::chrono::milliseconds timeout{
      config["testsuite-grpc-client-timeout-ms"].As<int>(30000)};

  return testsuite::GrpcControl(timeout, is_tls_enabled);
}

}  // namespace

TestsuiteSupport::TestsuiteSupport(const components::ComponentConfig& config,
                                   const components::ComponentContext&)
    : cache_control_(
          ParsePeriodicUpdatesMode(config["testsuite-periodic-update-enabled"]
                                       .As<std::optional<bool>>())),
      dump_control_(ParseDumpControl(config)),
      postgres_control_(ParsePostgresControl(config)),
      redis_control_(ParseRedisControl(config)),
      testsuite_tasks_(ParseTestsuiteTasks(config)),
      grpc_control_(ParseGrpcControl(config)) {}

TestsuiteSupport::~TestsuiteSupport() = default;

testsuite::CacheControl& TestsuiteSupport::GetCacheControl() {
  return cache_control_;
}

testsuite::ComponentControl& TestsuiteSupport::GetComponentControl() {
  return component_control_;
}

testsuite::DumpControl& TestsuiteSupport::GetDumpControl() {
  return dump_control_;
}

testsuite::PeriodicTaskControl& TestsuiteSupport::GetPeriodicTaskControl() {
  return periodic_task_control_;
}

testsuite::TestpointControl& TestsuiteSupport::GetTestpointControl() {
  return testpoint_control_;
}

const testsuite::PostgresControl& TestsuiteSupport::GetPostgresControl() {
  return postgres_control_;
}

const testsuite::RedisControl& TestsuiteSupport::GetRedisControl() {
  return redis_control_;
}

testsuite::TestsuiteTasks& TestsuiteSupport::GetTestsuiteTasks() {
  return *testsuite_tasks_;
}

testsuite::HttpAllowedUrlsExtra& TestsuiteSupport::GetHttpAllowedUrlsExtra() {
  return http_allowed_urls_extra_;
}

testsuite::GrpcControl& TestsuiteSupport::GetGrpcControl() {
  return grpc_control_;
}

yaml_config::Schema TestsuiteSupport::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<impl::ComponentBase>(R"(
type: object
description: Testsuite support component
additionalProperties: false
properties:
    testsuite-periodic-update-enabled:
        type: boolean
        description: whether caches update periodically
        defaultDescription: true
    testsuite-periodic-dumps-enabled:
        type: boolean
        description: whether dumpable components write dumps periodically
        defaultDescription: true
    testsuite-pg-execute-timeout:
        type: string
        description: execute timeout override for postgres
    testsuite-pg-statement-timeout:
        type: string
        description: statement timeout override for postgres
    testsuite-pg-readonly-master-expected:
        type: boolean
        description: mutes readonly master detection warning
        defaultDescription: false
    testsuite-redis-timeout-connect:
        type: string
        description: minimum connection timeout for redis
    testsuite-redis-timeout-single:
        type: string
        description: minimum single shard timeout for redis
    testsuite-redis-timeout-all:
        type: string
        description: minimum command timeout for redis
    testsuite-grpc-is-tls-enabled:
        type: boolean
        description: whether TLS should be enabled
    testsuite-grpc-client-timeout-ms:
        type: integer
        description: forced timeout on client requests
    testsuite-tasks-enabled:
        type: boolean
        description: Weather or not testsuite tasks are enabled
        defaultDescription: false
)");
}

void TestsuiteSupport::OnAllComponentsAreStopping() {
  testsuite_tasks_->CheckNoRunningTasks();
}

}  // namespace components

USERVER_NAMESPACE_END
