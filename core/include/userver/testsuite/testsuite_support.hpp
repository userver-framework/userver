#pragma once

/// @file userver/testsuite/testsuite_support.hpp
/// @brief @copybrief components::TestsuiteSupport

#include <userver/components/component_fwd.hpp>
#include <userver/testsuite/cache_control.hpp>
#include <userver/testsuite/component_control.hpp>
#include <userver/testsuite/dump_control.hpp>
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
/// testsuite-tasks-enabled | enable testsuite tasks facility | true if the corresponding testsuite component is available
///
/// ## Static configuration example:
///
/// @snippet components/common_component_list_test.cpp  Sample testsuite support component config

// clang-format on

class TestsuiteSupport final : public components::impl::ComponentBase {
 public:
  static constexpr std::string_view kName = "testsuite-support";

  TestsuiteSupport(const components::ComponentConfig& component_config,
                   const components::ComponentContext& component_context);
  ~TestsuiteSupport() override;

  testsuite::CacheControl& GetCacheControl();
  testsuite::ComponentControl& GetComponentControl();
  testsuite::DumpControl& GetDumpControl();
  testsuite::PeriodicTaskControl& GetPeriodicTaskControl();
  testsuite::TestpointControl& GetTestpointControl();
  const testsuite::PostgresControl& GetPostgresControl();
  const testsuite::RedisControl& GetRedisControl();
  testsuite::TestsuiteTasks& GetTestsuiteTasks();
  testsuite::HttpAllowedUrlsExtra& GetHttpAllowedUrlsExtra();

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  void OnAllComponentsAreStopping() override;

  testsuite::CacheControl cache_control_;
  testsuite::ComponentControl component_control_;
  testsuite::DumpControl dump_control_;
  testsuite::PeriodicTaskControl periodic_task_control_;
  testsuite::TestpointControl testpoint_control_;
  testsuite::PostgresControl postgres_control_;
  testsuite::RedisControl redis_control_;
  std::unique_ptr<testsuite::TestsuiteTasks> testsuite_tasks_;
  testsuite::HttpAllowedUrlsExtra http_allowed_urls_extra_;
};

template <>
inline constexpr bool kHasValidate<TestsuiteSupport> = true;

}  // namespace components

USERVER_NAMESPACE_END
