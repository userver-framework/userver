#include <components/minimal_server_component_list.hpp>
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
  "HTTP_CLIENT_CONNECTION_POOL_SIZE": 1000,
  "HTTP_CLIENT_CONNECT_THROTTLE": {
    "max-size": 100,
    "token-update-interval-ms": 0
  }
})~";

constexpr std::string_view kRuntimeConfigMissingParam = R"~({
  "USERVER_TASK_PROCESSOR_PROFILER_DEBUG": {},
  "USERVER_HTTP_PROXY": "",
  "USERVER_LOG_REQUEST": true,
  "USERVER_CHECK_AUTH_IN_HANDLERS": false,
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
  runtime_config_path: {1})",
                kTmpDir.GetPath(), kRuntimeConfingPath);

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
      bootstrap-path: $runtime_config_path
      fs-cache-path: $runtime_config_path  # May differ from bootstrap-path
      fs-task-processor: main-task-processor
    http-server-settings:
      # Nothing
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
  fs::blocking::RewriteFileContents(kRuntimeConfingPath, kRuntimeConfig);
  fs::blocking::RewriteFileContents(kConfigVariablesPath, kConfigVariables);

  components::RunOnce(components::InMemoryConfig{kStaticConfig},
                      components::MinimalServerComponentList());
}

TEST(CommonComponentList, ServerMinimalMissingRuntimeConfigParam) {
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
