#pragma once

/// @file userver/testsuite/testsuite_support.hpp
/// @brief @copybrief components::TestsuiteSupport

#include <userver/components/impl/component_base.hpp>
#include <userver/testsuite/cache_control.hpp>
#include <userver/testsuite/dump_control.hpp>
#include <userver/testsuite/grpc_control.hpp>
#include <userver/testsuite/http_allowed_urls_extra.hpp>
#include <userver/testsuite/periodic_task_control.hpp>
#include <userver/testsuite/postgres_control.hpp>
#include <userver/testsuite/redis_control.hpp>
#include <userver/testsuite/testpoint_control.hpp>

USERVER_NAMESPACE_BEGIN

/// Testsuite integration
namespace testsuite {
class TestsuiteTasks;
}

namespace components {

// clang-format off

/// @ingroup userver_components
///
/// @brief Testsuite support component
///
/// Provides additional functionality for testing, e.g. forced cache updates.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// testsuite-periodic-update-enabled | whether caches update periodically | true
/// testsuite-pg-execute-timeout | execute timeout override for postgres | -
/// testsuite-pg-statement-timeout | statement timeout override for postgres | -
/// testsuite-pg-readonly-master-expected | mutes readonly master detection warning | false
/// testsuite-redis-timeout-connect | minimum connection timeout for redis | -
/// testsuite-redis-timeout-single | minimum single shard timeout for redis | -
/// testsuite-redis-timeout-all | minimum command timeout for redis | -
/// testsuite-tasks-enabled | enable testsuite tasks facility | false
/// testsuite-increased-timeout | increase timeouts for connections, statement executions, RPC timeouts to avoid timeouts happening in testing environments, where the hardware differs from production. Overrides postgres, redis and grpc timeouts if these are missing | 0ms
/// cache-update-execution |  If 'sequential' the caches are updated by testsuite sequentially in the order for cache component registration, which makes sense if service has components that push value into a cache component. If 'concurrent' the caches are updated concurrently with respect to the cache component dependencies. | concurrent
///
/// ## Static configuration example:
///
/// @snippet components/common_component_list_test.cpp  Sample testsuite support component config

// clang-format on

class TestsuiteSupport final : public components::impl::ComponentBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of components::TestsuiteSupport
  static constexpr std::string_view kName = "testsuite-support";

  TestsuiteSupport(const components::ComponentConfig& component_config,
                   const components::ComponentContext& component_context);
  ~TestsuiteSupport() override;

  testsuite::CacheControl& GetCacheControl();
  testsuite::DumpControl& GetDumpControl();
  testsuite::PeriodicTaskControl& GetPeriodicTaskControl();
  testsuite::TestpointControl& GetTestpointControl();
  const testsuite::PostgresControl& GetPostgresControl();
  const testsuite::RedisControl& GetRedisControl();
  testsuite::TestsuiteTasks& GetTestsuiteTasks();
  testsuite::HttpAllowedUrlsExtra& GetHttpAllowedUrlsExtra();
  testsuite::GrpcControl& GetGrpcControl();
  /// @returns 0 if timeout was not increased via
  /// `testsuite-increased-timeout` static option,
  /// `testsuite-increased-timeout` value otherwise
  std::chrono::milliseconds GetIncreasedTimeout() const noexcept;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  void OnAllComponentsAreStopping() override;

  const std::chrono::milliseconds increased_timeout_;
  testsuite::CacheControl cache_control_;
  testsuite::DumpControl dump_control_;
  testsuite::PeriodicTaskControl periodic_task_control_;
  testsuite::TestpointControl testpoint_control_;
  testsuite::PostgresControl postgres_control_;
  testsuite::RedisControl redis_control_;
  std::unique_ptr<testsuite::TestsuiteTasks> testsuite_tasks_;
  testsuite::HttpAllowedUrlsExtra http_allowed_urls_extra_;
  testsuite::GrpcControl grpc_control_;
};

template <>
inline constexpr bool kHasValidate<TestsuiteSupport> = true;

}  // namespace components

USERVER_NAMESPACE_END
