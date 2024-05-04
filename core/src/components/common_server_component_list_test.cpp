#include <userver/components/common_server_component_list.hpp>

#include <fmt/format.h>

#include <userver/components/common_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/fs/blocking/temp_directory.hpp>  // for fs::blocking::TempDirectory
#include <userver/fs/blocking/write.hpp>  // for fs::blocking::RewriteFileContents
#include <userver/server/handlers/ping.hpp>

#include <components/component_list_test.hpp>
#include <userver/internal/net/net_listener.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kConfigVarsTemplate = R"(
  userver-dumps-root: {0}
  dynamic-config-cache-path: {1}
  log_level: {2}
  file_path: {3}
  overflow_behavior: {4}
  server-port: {5}
  server-monitor-port: {6}
)";

// We deliberately have some defaulted options explicitly specified here, for
// testing and documentation purposes.
// BEWARE! No separate fs-task-processor. Testing almost single thread mode
constexpr std::string_view kStaticConfig = R"(
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
    monitor-task-processor:
      thread_name: mon-worker
      worker_threads: 1
  components:
    manager-controller:  # Nothing
    logging:
      fs-task-processor: main-task-processor
      loggers:
        default:
          file_path: $file_path
          level: $log_level
          message_queue_size: 4  # small, to test queue overflows
          overflow_behavior: $overflow_behavior
    tracer:
        service-name: config-service
    statistics-storage:
      # Nothing
    dynamic-config:
      updates-enabled: true
      fs-cache-path: $dynamic-config-cache-path
      fs-task-processor: main-task-processor
    server:
      listener:
          port: $server-port
          task_processor: main-task-processor
      listener-monitor:
          connection:
              in_buffer_size: 32768
              requests_queue_size_threshold: 100
          port: $server-monitor-port
          port#fallback: 1188
          task_processor: monitor-task-processor
    auth-checker-settings:
      # Nothing
    logging-configurator:
      limited-logging-enable: true
      limited-logging-interval: 1s
    dump-configurator:
      dump-root: $userver-dumps-root
    testsuite-support:
    http-client:
      pool-statistics-disable: true
      thread-name-prefix: http-client
      threads: 2
      fs-task-processor: main-task-processor
      destination-metrics-auto-max-size: 100
      user-agent: common_component_list sample
      testsuite-enabled: true
      testsuite-timeout: 5s
      testsuite-allowed-url-prefixes: ['http://localhost:8083/', 'http://localhost:8084/']
    dns-client:
      fs-task-processor: main-task-processor
    dynamic-config-client:
      get-configs-overrides-for-service: true
      service-name: common_component_list-service
      http-timeout: 20s
      http-retries: 5
      config-url: http://localhost:8083/
    dynamic-config-client-updater:
      update-types: full-and-incremental
      update-interval: 5s
      update-jitter: 2s
      full-update-interval: 5m
      config-settings: false
      additional-cleanup-interval: 5m
      first-update-fail-ok: true
    tracer:
        service-name: config-service
    statistics-storage:
      # Nothing
    http-client-statistics:
      fs-task-processor: main-task-processor
    system-statistics-collector:
      fs-task-processor: main-task-processor
      with-nginx: false
# /// [Sample tests control component config]
# yaml
    tests-control:
        skip-unregistered-testpoints: true
        testpoint-timeout: 10s
        testpoint-url: https://localhost:7891/testpoint

        # Some options from server::handlers::HttpHandlerBase
        path: /tests/{action}
        method: POST
        task_processor: main-task-processor
# /// [Sample tests control component config]
# /// [Sample congestion control component config]
# yaml
    congestion-control:
        fake-mode: true
        min-cpu: 2
        only-rtc: false
        # Uncomment if you want to change status code for ratelimited responses
        # status-code: 503

        # Common component options
        load-enabled: true
# /// [Sample congestion control component config]
# /// [Sample handler ping component config]
# yaml
    handler-ping:
        # Options from server::handlers::HandlerBase
        path: /ping
        method: GET
        task_processor: main-task-processor
        max_request_size: 256
        max_headers_size: 256
        parse_args_from_body: false
        # auth: TODO:
        url_trailing_slash: strict-match
        max_requests_in_flight: 100
        request_body_size_log_limit: 128
        response_data_size_log_limit: 128
        max_requests_per_second: 100
        decompress_request: false
        throttling_enabled: false
        set-response-server-hostname: false

        # Options from server::handlers::HttpHandlerBase
        log-level: WARNING

        # Common component options
        load-enabled: true
# /// [Sample handler ping component config]
# /// [Sample handler log level component config]
# yaml
    handler-log-level:
        path: /service/log-level/{level}
        method: GET,PUT
        task_processor: monitor-task-processor
# /// [Sample handler log level component config]
# yaml
    handler-on-log-rotate:
        path: /service/on-log-rotate/
        method: POST
        task_processor: monitor-task-processor
# /// [Sample handler on log rotate component config]
# /// [Sample handler inspect requests component config]
# yaml
    handler-inspect-requests:
        path: /service/inspect-requests
        method: GET
        task_processor: monitor-task-processor
# /// [Sample handler inspect requests component config]
# /// [Sample handler implicit http options component config]
# yaml
    handler-implicit-http-options:
        as_fallback: implicit-http-options
        method: OPTIONS
        task_processor: main-task-processor
# /// [Sample handler implicit http options component config]
# /// [Sample handler jemalloc component config]
# yaml
    handler-jemalloc:
        path: /service/jemalloc/prof/{command}
        method: POST
        task_processor: monitor-task-processor
# /// [Sample handler jemalloc component config]
# /// [Sample handler dns client control component config]
# yaml
    handler-dns-client-control:
        path: /service/dnsclient/{command}
        method: POST
        task_processor: monitor-task-processor
# /// [Sample handler dns client control component config]
# /// [Sample handler server monitor component config]
# yaml
    handler-server-monitor:
        path: /statistics
        method: GET
        task_processor: monitor-task-processor
        common-labels:
            application: sample application
            zone: some
        format: json
# /// [Sample handler server monitor component config]
# /// [Sample handler dynamic debug log component config]
    handler-dynamic-debug-log:
        path: /service/log/dynamic-debug
        method: GET,PUT,DELETE
        task_processor: monitor-task-processor
# /// [Sample handler dynamic debug log component config]
config_vars: )";

struct ServicePorts final {
  std::uint16_t server_port{0};
  std::uint16_t monitor_port{0};
};

ServicePorts FindFreePorts() {
  ServicePorts ports;
  engine::RunStandalone([&ports] {
    const internal::net::TcpListener listener1;
    const internal::net::TcpListener listener2;
    ports = {listener1.Port(), listener2.Port()};
  });
  return ports;
}

class CommonServerComponentList : public ComponentList {
 protected:
  CommonServerComponentList() {
    fs::blocking::RewriteFileContents(
        GetDynamicConfigCachePath(),
        formats::json::ToString(
            dynamic_config::impl::GetDefaultDocsMap().AsJson()));
  }

  std::string GetDynamicConfigCachePath() const {
    return temp_root_.GetPath() + "/config_cache.json";
  }

  std::string GetConfigVarsPath() const {
    return temp_root_.GetPath() + "/config_vars.json";
  }

  std::string GetDumpsRoot() const { return temp_root_.GetPath(); }

  std::string GetLogsPath() const { return temp_root_.GetPath() + "/logs.txt"; }

  ServicePorts GetPorts() const { return ports_; }

 private:
  const fs::blocking::TempDirectory temp_root_{
      fs::blocking::TempDirectory::Create()};
  const ServicePorts ports_ = FindFreePorts();
};

}  // namespace

TEST_F(CommonServerComponentList, Smoke) {
  fs::blocking::RewriteFileContents(
      GetConfigVarsPath(),
      fmt::format(kConfigVarsTemplate, GetDumpsRoot(),
                  GetDynamicConfigCachePath(), "warning", "'@null'", "discard",
                  GetPorts().server_port, GetPorts().monitor_port));

  components::RunOnce(
      components::InMemoryConfig{std::string{kStaticConfig} +
                                 GetConfigVarsPath()},
      components::CommonComponentList()
          .AppendComponentList(components::CommonServerComponentList())
          .Append<server::handlers::Ping>());
}

TEST_F(CommonServerComponentList, TraceLogging) {
  fs::blocking::RewriteFileContents(
      GetConfigVarsPath(),
      fmt::format(kConfigVarsTemplate, GetDumpsRoot(),
                  GetDynamicConfigCachePath(), "trace", GetLogsPath(),
                  "discard", GetPorts().server_port, GetPorts().monitor_port));

  components::RunOnce(
      components::InMemoryConfig{std::string{kStaticConfig} +
                                 GetConfigVarsPath()},
      components::CommonComponentList()
          .AppendComponentList(components::CommonServerComponentList())
          .Append<server::handlers::Ping>());
}

TEST_F(CommonServerComponentList, NullLogging) {
  fs::blocking::RewriteFileContents(
      GetConfigVarsPath(),
      fmt::format(kConfigVarsTemplate, GetDumpsRoot(),
                  GetDynamicConfigCachePath(), "trace", "'@null'", "discard",
                  GetPorts().server_port, GetPorts().monitor_port));

  components::RunOnce(
      components::InMemoryConfig{std::string{kStaticConfig} +
                                 GetConfigVarsPath()},
      components::CommonComponentList()
          .AppendComponentList(components::CommonServerComponentList())
          .Append<server::handlers::Ping>());
}

TEST_F(CommonServerComponentList, BlockingDefaultLogger) {
  fs::blocking::RewriteFileContents(
      GetConfigVarsPath(),
      fmt::format(kConfigVarsTemplate, GetDumpsRoot(),
                  GetDynamicConfigCachePath(), "warning", "'@null'", "block",
                  GetPorts().server_port, GetPorts().monitor_port));

  const components::InMemoryConfig config{std::string{kStaticConfig} +
                                          GetConfigVarsPath()};
  const auto component_list =
      components::CommonComponentList()
          .AppendComponentList(components::CommonServerComponentList())
          .Append<server::handlers::Ping>();
  UEXPECT_THROW_MSG(components::RunOnce(config, component_list), std::exception,
                    "efault logger");
}

USERVER_NAMESPACE_END
