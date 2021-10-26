#include <cache/lru_cache_component_base_test.hpp>

#include <userver/components/minimal_component_list.hpp>
#include <userver/components/run.hpp>

#include <gtest/gtest.h>
#include <components/component_list_test.hpp>
#include <userver/components/tracer.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/blocking/write.hpp>

USERVER_NAMESPACE_BEGIN
namespace {

const auto kTmpDir = fs::blocking::TempDirectory::Create();
const std::string kRuntimeConfingPath =
    kTmpDir.GetPath() + "/runtime_config.json";
const std::string kConfigVariablesPath =
    kTmpDir.GetPath() + "/config_vars.json";

const std::string kConfigVariables =
    fmt::format("runtime_config_path: {}", kRuntimeConfingPath);

// BEWARE! No separate fs-task-processor. Testing almost single thread mode
const std::string kStaticConfig = R"(
components_manager:
  coro_pool:
    initial_size: 50
    max_size: 500
  default_task_processor: main-task-processor
  event_thread_pool:
    threads: 1
  task_processors:
    main-task-processor:
      thread_name: main-worker
      worker_threads: 1
  components:
    manager-controller:  # Nothing
# /// [Sample lru cache component config]
# yaml
    example-cache:
      size: 1
      ways: 1
      lifetime: 1s # 0 (unlimited) by default
      config-settings: false # true by default
# /// [Sample lru cache component config]
    logging:
      fs-task-processor: main-task-processor
      loggers:
        default:
          file_path: '@null'
    tracer:
        service-name: config-service
    statistics-storage:
      # Nothing
    testsuite-support:
      testsuite-periodic-update-enabled: true
      testsuite-pg-execute-timeout: 300ms
      testsuite-pg-statement-timeout: 300ms
      testsuite-pg-readonly-master-expected: false
      testsuite-redis-timeout-connect: 5s
      testsuite-redis-timeout-single: 1s
      testsuite-redis-timeout-all: 750ms
    taxi-config:
      fs-cache-path: $runtime_config_path
      fs-task-processor: main-task-processor
    taxi-config-fallbacks:
      fallback-path: $runtime_config_path
config_vars: )" + kConfigVariablesPath +
                                  R"(
)";

}  // namespace

TEST(LruCacheComponent, ExampleCacheComponent) {
  /// [Sample lru cache component registration]
  auto component_list = components::MinimalComponentList();
  component_list.Append<ExampleCacheComponent>();
  /// [Sample lru cache component registration]
  component_list.Append<components::TestsuiteSupport>();

  tests::LogLevelGuard guard;

  fs::blocking::RewriteFileContents(kRuntimeConfingPath, tests::kRuntimeConfig);
  fs::blocking::RewriteFileContents(kConfigVariablesPath, kConfigVariables);

  components::RunOnce(components::InMemoryConfig{kStaticConfig},
                      component_list);
}

USERVER_NAMESPACE_END
