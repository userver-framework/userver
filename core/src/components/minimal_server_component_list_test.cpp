#include <userver/components/minimal_server_component_list.hpp>

#include <fmt/format.h>

#include <userver/components/run.hpp>
#include <userver/fs/blocking/temp_directory.hpp>  // for fs::blocking::TempDirectory
#include <userver/fs/blocking/write.hpp>  // for fs::blocking::RewriteFileContents

#include <components/component_list_test.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kRuntimeConfigMissingParam = R"~({
  "USERVER_TASK_PROCESSOR_PROFILER_DEBUG": {},
  "USERVER_LOG_REQUEST": true,
  "USERVER_CHECK_AUTH_IN_HANDLERS": false,
  "USERVER_HTTP_PROXY": "",
  "USERVER_CANCEL_HANDLE_REQUEST_BY_DEADLINE": false,
  "USERVER_NO_LOG_SPANS":{"names":[], "prefixes":[]},
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
  "USERVER_CACHES": {},
  "USERVER_LRU_CACHES": {},
  "USERVER_DUMPS": {},
  "HTTP_CLIENT_CONNECTION_POOL_SIZE": 1000,
  "HTTP_CLIENT_CONNECT_THROTTLE": {
    "max-size": 100,
    "token-update-interval-ms": 0
  },
  "HTTP_CLIENT_ENFORCE_TASK_DEADLINE": {
    "cancel-request": false,
    "update-timeout": false
  },
  "USERVER_RPS_CCONTROL_ENABLED": true,
  "USERVER_RPS_CCONTROL": {
    "down-level": 8,
    "down-rate-percent": 1,
    "load-limit-crit-percent": 50,
    "load-limit-percent": 0,
    "min-limit": 2,
    "no-limit-seconds": 300,
    "overload-off-seconds": 8,
    "overload-on-seconds": 8,
    "up-level": 2,
    "up-rate-percent": 1
  },
  "USERVER_RPS_CCONTROL_CUSTOM_STATUS":{},
  "SAMPLE_INTEGER_FROM_RUNTIME_CONFIG": 42
})~";

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
    threads: 4
  task_processors:
    main-task-processor:
      thread_name: main-worker
      worker_threads: 4
  components:
    manager-controller:  # Nothing
    logging:
      fs-task-processor: main-task-processor
      loggers:
        default:
          file_path: '@stderr'
    tracer:
        service-name: config-service
    statistics-storage:
      # Nothing
    taxi-config:
      fs-cache-path: $runtime_config_path
      fs-task-processor: main-task-processor
    taxi-config-fallbacks:
        fallback-path: $runtime_config_path
    server:
      listener:
          port: 8087
          task_processor: main-task-processor
    auth-checker-settings:
      # Nothing
config_vars: )" + kConfigVariablesPath +
                                  R"(
)";

}  // namespace

TEST(CommonComponentList, ServerMinimal) {
  tests::LogLevelGuard logger_guard{};
  fs::blocking::RewriteFileContents(kRuntimeConfingPath, tests::kRuntimeConfig);
  fs::blocking::RewriteFileContents(kConfigVariablesPath, kConfigVariables);

  components::RunOnce(components::InMemoryConfig{kStaticConfig},
                      components::MinimalServerComponentList());
}

TEST(CommonComponentList, ServerMinimalMissingRuntimeConfigParam) {
  tests::LogLevelGuard logger_guard{};
  fs::blocking::RewriteFileContents(kRuntimeConfingPath,
                                    kRuntimeConfigMissingParam);
  fs::blocking::RewriteFileContents(kConfigVariablesPath, kConfigVariables);

  try {
    components::RunOnce(components::InMemoryConfig{kStaticConfig},
                        components::MinimalServerComponentList());
    FAIL() << "Missing runtime config value was not reported";
  } catch (const std::runtime_error& e) {
    EXPECT_NE(std::string_view{e.what()}.find("USERVER_LOG_REQUEST_HEADERS"),
              std::string_view::npos)
        << "'USERVER_LOG_REQUEST_HEADERS' is missing in error message: "
        << e.what();
  }
}

USERVER_NAMESPACE_END
