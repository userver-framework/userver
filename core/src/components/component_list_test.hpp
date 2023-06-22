#pragma once

#include <string_view>

#include <userver/engine/run_standalone.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/opentracing.hpp>
#include <userver/tracing/tracer.hpp>

#include <userver/utest/default_logger_fixture.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace tests {

namespace impl {

class DefaultLoggerGuardTest {
 public:
  DefaultLoggerGuardTest() noexcept
      : logger_prev_(logging::impl::DefaultLoggerRef()),
        log_level_scope_(logging::GetLoggerLevel(logger_prev_)) {}

  ~DefaultLoggerGuardTest() {
    logging::impl::SetDefaultLoggerRef(logger_prev_);
  }

 private:
  logging::LoggerRef logger_prev_;
  logging::DefaultLoggerLevelScope log_level_scope_;
};

}  // namespace impl

inline constexpr std::string_view kRuntimeConfig = R"~({
  "USERVER_BAGGAGE_ENABLED": false,
  "BAGGAGE_SETTINGS": {
    "allowed_keys": []
  },
  "USERVER_TASK_PROCESSOR_PROFILER_DEBUG": {},
  "USERVER_LOG_REQUEST": true,
  "USERVER_LOG_REQUEST_HEADERS": false,
  "USERVER_CHECK_AUTH_IN_HANDLERS": false,
  "USERVER_CANCEL_HANDLE_REQUEST_BY_DEADLINE": false,
  "USERVER_HTTP_PROXY": "",
  "USERVER_NO_LOG_SPANS":{"names":[], "prefixes":[]},
  "USERVER_LOG_DYNAMIC_DEBUG": {"force-enabled":[], "force-disabled":[]},
  "USERVER_HANDLER_STREAM_API_ENABLED": true,
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
  "USERVER_RPS_CCONTROL_ACTIVATED_FACTOR_METRIC": 5,
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
  "USERVER_RPS_CCONTROL_CUSTOM_STATUS":{},
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
  "SAMPLE_INTEGER_FROM_RUNTIME_CONFIG": 42,
  "DYNAMIC_CONFIG_UPDATES_SINK_CHAIN": ""
})~";

// BEWARE! No separate fs-task-processor. Testing almost single thread mode
inline constexpr std::string_view kMinimalStaticConfig = R"(
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
    logging:
      fs-task-processor: main-task-processor
      loggers:
        default:
          file_path: $logger_file_path
          format: ltsv
    tracer:
        service-name: config-service
    statistics-storage:
      # Nothing
    dynamic-config:
      fs-cache-path: $runtime_config_path
      fs-task-processor: main-task-processor
# /// [Sample dynamic config fallback component]
# yaml
    dynamic-config-fallbacks:
      fallback-path: $runtime_config_path
# /// [Sample dynamic config fallback component]
config_vars: )";

struct TracingGuard final {
  TracingGuard()
      : opentracing_logger(tracing::OpentracingLogger()),
        tracer(tracing::Tracer::GetTracer()) {}

  ~TracingGuard() {
    if (tracing::Tracer::GetTracer() != tracer ||
        tracing::OpentracingLogger() != opentracing_logger) {
      engine::RunStandalone([&] {
        tracing::Tracer::SetTracer(tracer);
        tracing::SetOpentracingLogger(opentracing_logger);
      });
    }
  }

  const logging::LoggerPtr opentracing_logger;
  const tracing::TracerPtr tracer;
};

}  // namespace tests

class ComponentList : public ::testing::Test {
  tests::impl::DefaultLoggerGuardTest default_logger_guard_;
  tests::TracingGuard tracing_guard_;
};

USERVER_NAMESPACE_END
