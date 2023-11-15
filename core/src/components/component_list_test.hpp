#pragma once

#include <string_view>

#include <userver/engine/run_standalone.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/tracer.hpp>

#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/utest/default_logger_fixture.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace tests {

namespace impl {

class DefaultLoggerGuardTest {
 public:
  DefaultLoggerGuardTest() noexcept
      : logger_prev_(logging::GetDefaultLogger()),
        log_level_scope_(logging::GetLoggerLevel(logger_prev_)) {}

  ~DefaultLoggerGuardTest() {
    logging::impl::SetDefaultLoggerRef(logger_prev_);
  }

 private:
  logging::LoggerRef logger_prev_;
  logging::DefaultLoggerLevelScope log_level_scope_;
};

}  // namespace impl

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
      worker_threads: 1
  components:
    logging:
      fs-task-processor: main-task-processor
      loggers:
        default:
          file_path: $logger_file_path
          format: ltsv
config_vars: )";

struct TracingGuard final {
  TracingGuard() : tracer(tracing::Tracer::GetTracer()) {}

  ~TracingGuard() {
    if (tracing::Tracer::GetTracer() != tracer) {
      engine::RunStandalone([&] { tracing::Tracer::SetTracer(tracer); });
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
