#include <userver/components/common_component_list.hpp>

#include <fmt/format.h>

#include <userver/components/run.hpp>
#include <userver/fs/blocking/temp_directory.hpp>  // for fs::blocking::TempDirectory
#include <userver/fs/blocking/write.hpp>  // for fs::blocking::RewriteFileContents

#include <components/component_list_test.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kConfigVarsTemplate = R"(
  userver-dumps-root: {0}
  runtime_config_path: {1}
  access_log_path: {0}/access.log
  access_tskv_log_path: {0}/access_tskv.log
  default_log_path: {0}/server.log
  log_level: {2}
)";

// clang-format off
constexpr std::string_view kStaticConfig = R"(
# /// [Sample components manager config component config]
# yaml
components_manager:
  coro_pool:
    initial_size: 5000
    max_size: 50000
  default_task_processor: main-task-processor
  event_thread_pool:
    threads: 2
  task_processors:
    bg-task-processor:
      thread_name: bg-worker
      worker_threads: 2
      os-scheduling: idle
      task-trace:
        every: 1000
        max-context-switch-count: 1000
        logger: tracer
    fs-task-processor:
      thread_name: fs-worker
      worker_threads: 2
    main-task-processor:
      thread_name: main-worker
      worker_threads: 16
    monitor-task-processor:
      thread_name: monitor
      worker_threads: 2
  components:
    manager-controller:  # Nothing
# /// [Sample components manager config component config]
# /// [Sample logging configurator component config]
# yaml
    logging-configurator:
      limited-logging-enable: true
      limited-logging-interval: 1s
# /// [Sample logging configurator component config]
# /// [Sample dump configurator component config]
# yaml
    dump-configurator:
      dump-root: $userver-dumps-root
# /// [Sample dump configurator component config]
# /// [Sample testsuite support component config]
# yaml
    testsuite-support:
      testsuite-periodic-update-enabled: true
      testsuite-pg-execute-timeout: 300ms
      testsuite-pg-statement-timeout: 300ms
      testsuite-pg-readonly-master-expected: false
      testsuite-redis-timeout-connect: 5s
      testsuite-redis-timeout-single: 1s
      testsuite-redis-timeout-all: 750ms
# /// [Sample testsuite support component config]
# /// [Sample http client component config]
# yaml
    http-client:
      pool-statistics-disable: false
      thread-name-prefix: http-client
      threads: 2
      defer-events: false
      fs-task-processor: fs-task-processor
      destination-metrics-auto-max-size: 100
      user-agent: common_component_list sample
      testsuite-enabled: true
      testsuite-timeout: 5s
      testsuite-allowed-url-prefixes: ['http://localhost:8083/', 'http://localhost:8084/']
# /// [Sample http client component config]
# /// [Sample dns client component config]
# yaml
    dns-client:
      fs-task-processor: fs-task-processor
      hosts-file-path: /etc/hosts
      hosts-file-update-interval: 5m
      network-timeout: 1s
      network-attempts: 1
      network-custom-servers:
        - 127.0.0.1
        - 127.0.0.2
      cache-ways: 16
      cache-size-per-way: 256
      cache-max-reply-ttl: 5m
      cache-failure-ttl: 5s
# /// [Sample dns client component config]
# /// [Sample dynamic configs client component config]
# yaml
    dynamic-config-client:
      get-configs-overrides-for-service: true
      service-name: common_component_list-service
      http-timeout: 20s
      http-retries: 5
      config-url: http://localhost:8083/
      configs-stage: $configs_stage
      fallback-to-no-proxy: false
# /// [Sample dynamic configs client component config]
# /// [Sample dynamic config client updater component config]
# yaml
    dynamic-config-client-updater:
      store-enabled: true
      load-only-my-values: true
      fallback-path: $runtime_config_path
      fallback-path#fallback: /some/path/to/runtime_config.json
      fs-task-processor: fs-task-processor

      # options from components::CachingComponentBase
      update-types: full-and-incremental
      update-interval: 5s
      update-jitter: 2s
      full-update-interval: 5m
      first-update-fail-ok: true
      config-settings: true
      additional-cleanup-interval: 5m
      testsuite-force-periodic-update: true
# /// [Sample dynamic config client updater component config]
# /// [Sample logging component config]
# yaml
    logging:
      fs-task-processor: fs-task-processor
      loggers:
        access:
          file_path: $access_log_path
          overflow_behavior: discard
          format: raw
        access-tskv:
          file_path: $access_tskv_log_path
          overflow_behavior: discard
          format: raw
        default:
          file_path: $default_log_path
          level: $log_level
          level#fallback: debug
          overflow_behavior: discard
        tracer:
          file_path: '@stdout'
          overflow_behavior: discard
# /// [Sample logging component config]
# /// [Sample tracer component config]
# yaml
    tracer:
        service-name: config-service
        tracer: native
# /// [Sample tracer component config]
# /// [Sample statistics storage component config]
# yaml
    statistics-storage:
      # Nothing
# /// [Sample statistics storage component config]
# /// [Sample dynamic config component config]
# yaml
    dynamic-config:
      fs-cache-path: $runtime_config_path
      fs-task-processor: fs-task-processor
# /// [Sample dynamic config component config]
    http-client-statistics:
      fs-task-processor: fs-task-processor
# /// [Sample system statistics component config]
# yaml
    system-statistics-collector:
      fs-task-processor: fs-task-processor
      with-nginx: false
# /// [Sample system statistics component config]
config_vars: )";
// clang-format on

}  // namespace

TEST_F(ComponentList, Common) {
  const auto temp_root = fs::blocking::TempDirectory::Create();
  const std::string runtime_config_path =
      temp_root.GetPath() + "/dynamic_config.json";
  const std::string config_vars_path =
      temp_root.GetPath() + "/config_vars.json";

  fs::blocking::RewriteFileContents(runtime_config_path, tests::kRuntimeConfig);
  fs::blocking::RewriteFileContents(
      config_vars_path,
      fmt::format(kConfigVarsTemplate, temp_root.GetPath(), runtime_config_path,
                  ToString(logging::GetDefaultLoggerLevel())));

  components::RunOnce(
      components::InMemoryConfig{std::string{kStaticConfig} + config_vars_path},
      components::CommonComponentList(), temp_root.GetPath() + "/server.log");
}

USERVER_NAMESPACE_END
