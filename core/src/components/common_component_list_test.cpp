#include <components/common_component_list.hpp>
#include <components/run.hpp>
#include <fs/blocking/temp_directory.hpp>  // for fs::blocking::TempDirectory
#include <fs/blocking/write.hpp>  // for fs::blocking::RewriteFileContents

#include <utest/utest.hpp>

namespace {

constexpr std::string_view kRuntimeConfig = R"~({
  "USERVER_TASK_PROCESSOR_PROFILER_DEBUG": {},
  "USERVER_LOG_REQUEST": true,
  "USERVER_LOG_REQUEST_HEADERS": false,
  "USERVER_CHECK_AUTH_IN_HANDLERS": false,
  "USERVER_HTTP_PROXY": "",
  "USERVER_NO_LOG_SPANS":{"names":["foo"], "prefixes":[]},
  "USERVER_TASK_PROCESSOR_QOS": {
    "default-service": {
      "default-task-processor": {
        "wait_queue_overload": {
          "action": "ignore",
          "length_limit": 5000,
          "time_limit_us": 3000
        }
      }
    }
  },
  "HTTP_CLIENT_CONNECTION_POOL_SIZE": 1000,
  "HTTP_CLIENT_CONNECT_THROTTLE": {
    "max-size": 100,
    "token-update-interval-ms": 0
  }
})~";

const auto kTmpDir = fs::blocking::TempDirectory::Create();
const std::string kRuntimeConfingPath =
    kTmpDir.GetPath() + "/runtime_config.json";
const std::string kConfigVariablesPath =
    kTmpDir.GetPath() + "/config_vars.json";

// TODO: purge userver-cache-dump-path after TAXICOMMON-3540
const std::string kConfigVariables =
    fmt::format(R"(
  userver-cache-dump-path: {0}
  runtime_config_path: {1}
  access_log_path: {0}/access.log
  access_tskv_log_path: {0}/access_tskv.log
  default_log_path: {0}/server.log)",
                kTmpDir.GetPath(), kRuntimeConfingPath);

// clang-format off
const std::string kStaticConfig = R"(
# /// [Sample components manager config component config]
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
    fs-task-processor:
      thread_name: fs-worker
      worker_threads: 2
    main-task-processor:
      thread_name: main-worker
      worker_threads: 16
    monitor-task-processor:
      thread_name: monitor
      worker_threads: 2
    pg-task-processor:
      thread_name: pg-worker
      worker_threads: 2
  components:
    manager-controller:  # Nothing
# /// [Sample components manager config component config]
# /// [Sample logging configurator component config]
    logging-configurator:
      limited-logging-enable: true
      limited-logging-interval: 1s
# /// [Sample logging configurator component config]
# /// [Sample testsuite support component config]
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
    http-client:
      pool-statistics-disable: false
      thread-name-prefix: http-client
      threads: 2
      fs-task-processor: fs-task-processor
      destination-metrics-auto-max-size: 100
      user-agent: common_component_list sample
      testsuite-enabled: true
      testsuite-timeout: 5s
      testsuite-allowed-url-prefixes: 'http://localhost:8083/ http://localhost:8084/'
# /// [Sample http client component config]
# /// [Sample taxi configs client component config]
    taxi-configs-client:
      get-configs-overrides-for-service: true
      service-name: common_component_list-service
      http-timeout: 20s
      http-retries: 5
      config-url: http://localhost:8083/
      use-uconfigs: $use_uconfigs
      use-uconfigs#fallback: false
      uconfigs-url: http://localhost:8084/
      fallback-to-no-proxy: false
# /// [Sample taxi configs client component config]
# /// [Sample taxi config client updater component config]
    taxi-config-client-updater:
      store-enabled: true
      load-only-my-values: true
      fallback-path: $runtime_config_path
      fallback-path#fallback: /some/path/to/runtime_config.json

      # options from components::CachingComponentBase
      update-types: full-and-incremental
      update-interval: 5s
      update-jitter: 2s
      full-update-interval: 5m
      first-update-fail-ok: true
      config-settings: true
      additional-cleanup-interval: 5m
      testsuite-force-periodic-update: true
# /// [Sample taxi config client updater component config]
# /// [Sample logging component config]
    logging:
      fs-task-processor: fs-task-processor
      loggers:
        access:
          file_path: $access_log_path
          overflow_behavior: discard
          pattern: '[%Y-%m-%d %H:%M:%S.%f %z] %v'
        access-tskv:
          file_path: $access_tskv_log_path
          overflow_behavior: discard
          pattern: "tskv\ttskv_format=taxi_device_notify\ttimestamp=%Y-%m-%dT%H:%M:%S\t\
            timezone=%z%v"
        default:
          file_path: $default_log_path
          level: debug
          overflow_behavior: discard
# /// [Sample logging component config]
# /// [Sample tracer component config]
    tracer:
        service-name: config-service
        tracer: native
# /// [Sample tracer component config]
# /// [Sample statistics storage component config]
    statistics-storage:
      # Nothing
# /// [Sample statistics storage component config]
# /// [Sample taxi config component config]
    taxi-config:
      bootstrap-path: $runtime_config_path
      fs-cache-path: $runtime_config_path  # May differ from bootstrap-path
      fs-task-processor: fs-task-processor
# /// [Sample taxi config component config]
    http-client-statistics:
      fs-task-processor: fs-task-processor
# /// [Sample system statistics component config]
    system-statistics-collector:
      fs-task-processor: fs-task-processor
      update-interval: 1m
      with-nginx: false
# /// [Sample system statistics component config]
config_vars: )" + kConfigVariablesPath + R"(
)";
// clang-format on

}  // namespace

TEST(CommonComponentList, Common) {
  fs::blocking::RewriteFileContents(kRuntimeConfingPath, kRuntimeConfig);
  fs::blocking::RewriteFileContents(kConfigVariablesPath, kConfigVariables);

  components::RunOnce(components::InMemoryConfig{kStaticConfig},
                      components::CommonComponentList());
}
