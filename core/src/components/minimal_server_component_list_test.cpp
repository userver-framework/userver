#include <userver/components/minimal_server_component_list.hpp>

#include <fmt/format.h>
#include <gmock/gmock.h>

#include <userver/compiler/demangle.hpp>
#include <userver/components/component.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/components/manager_controller_component.hpp>
#include <userver/components/run.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/temp_directory.hpp>  // for fs::blocking::TempDirectory
#include <userver/fs/blocking/write.hpp>  // for fs::blocking::RewriteFileContents
#include <userver/logging/component.hpp>
#include <userver/utils/async.hpp>

#include <components/component_list_test.hpp>
#include <userver/internal/net/net_listener.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

std::uint16_t FindFreePort() {
  std::uint16_t result{};
  engine::RunStandalone([&result] {
    const internal::net::TcpListener listener{};
    result = listener.Port();
  });
  return result;
}

constexpr std::string_view kStaticConfig = R"(
components_manager:
  coro_pool:
    initial_size: 50
    max_size: 500
  default_task_processor: main-task-processor
  event_thread_pool:
    threads: 4
# /// [Sample task-switch tracing]
# yaml
  task_processors:
    fs-task-processor:
      worker_threads: 2
    main-task-processor:
      worker_threads: 4
      task-trace:
        every: 1
        max-context-switch-count: 50
        logger: tracer
  components:
    logging:
      fs-task-processor: fs-task-processor
      loggers:
        tracer:
          file_path: $tracer_log_path
          file_path#fallback: '@null'
          level: $tracer_level  # set to debug to get stacktraces
          level#fallback: info
# /// [Sample task-switch tracing]
        default:
          file_path: $default-logger-path
          file_path#fallback: '@null'
          level: warning
    dynamic-config:
      defaults: $dynamic-config-default-overrides
      defaults#fallback: {}
    server:
      listener:
          port: $server-port
          task_processor: main-task-processor
config_vars: )";

class ServerMinimalComponentList : public ComponentList {
 protected:
  const std::string& GetTempRoot() const { return temp_root_.GetPath(); }

  std::string GetConfigVarsPath() const {
    return temp_root_.GetPath() + "/config_vars.yaml";
  }

  const std::string& GetStaticConfig() const { return static_config_; }

  std::uint16_t GetServerPort() const { return server_port_; }

 private:
  fs::blocking::TempDirectory temp_root_ =
      fs::blocking::TempDirectory::Create();
  std::uint16_t server_port_ = FindFreePort();
  std::string static_config_ = std::string{kStaticConfig} + GetConfigVarsPath();
};

class TaskTraceProducer final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "task-trace-producer";

  TaskTraceProducer(const components::ComponentConfig& config,
                    const components::ComponentContext& context)
      : components::LoggableComponentBase(config, context) {
    // Task tracing is set up by ManagerControllerComponent.
    // It may not work in constructors of components that don't depend on it.
    [[maybe_unused]] const auto& manager_controller_component =
        context.FindComponent<components::ManagerControllerComponent>();

    // Task tracing is guaranteed to work for this task.
    utils::Async(std::string{kName}, [] { engine::Yield(); }).Get();
  }
};

}  // namespace

template <>
inline constexpr auto components::kConfigFileMode<TaskTraceProducer> =
    ConfigFileMode::kNotRequired;

TEST_F(ServerMinimalComponentList, Basic) {
  constexpr std::string_view kConfigVarsTemplate = R"(
    server-port: {0}
  )";
  const auto config_vars = fmt::format(kConfigVarsTemplate, GetServerPort());

  fs::blocking::RewriteFileContents(GetConfigVarsPath(), config_vars);

  components::RunOnce(components::InMemoryConfig{GetStaticConfig()},
                      components::MinimalServerComponentList());
}

TEST_F(ServerMinimalComponentList, TraceSwitching) {
  constexpr std::string_view kConfigVarsTemplate = R"(
    tracer_log_path: {0}
    server-port: {1}
  )";
  const std::string logs_path = GetTempRoot() + "/tracing_log.txt";
  const auto config_vars =
      fmt::format(kConfigVarsTemplate, logs_path, GetServerPort());

  fs::blocking::RewriteFileContents(GetConfigVarsPath(), config_vars);

  components::RunOnce(
      components::InMemoryConfig{GetStaticConfig()},
      components::MinimalServerComponentList().Append<TaskTraceProducer>());

  logging::LogFlush();

  const auto logs = fs::blocking::ReadFileContents(logs_path);
  // Assert not to print all logs multiple times on failure.
  ASSERT_THAT(logs, testing::HasSubstr(" changed state to kQueued"));
  ASSERT_THAT(logs, testing::HasSubstr(" changed state to kRunning"));
  ASSERT_THAT(logs, testing::HasSubstr(" changed state to kCompleted"));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("stacktrace= 0# ")));
}

TEST_F(ServerMinimalComponentList, TraceStacktraces) {
  constexpr std::string_view kConfigVarsTemplate = R"(
    tracer_log_path: {0}
    tracer_level: debug
    server-port: {1}
  )";
  const std::string logs_path = GetTempRoot() + "/tracing_st_log.txt";

  fs::blocking::RewriteFileContents(
      GetConfigVarsPath(),
      fmt::format(kConfigVarsTemplate, logs_path, GetServerPort()));

  components::RunOnce(
      components::InMemoryConfig{GetStaticConfig()},
      components::MinimalServerComponentList().Append<TaskTraceProducer>());

  logging::LogFlush();

  const auto logs = fs::blocking::ReadFileContents(logs_path);
  ASSERT_THAT(logs, testing::HasSubstr(" changed state to kQueued"));
  ASSERT_THAT(logs, testing::HasSubstr(" changed state to kRunning"));
  ASSERT_THAT(logs, testing::HasSubstr(" changed state to kCompleted"));
  ASSERT_THAT(logs, testing::HasSubstr("stacktrace= 0# "));
}

TEST_F(ServerMinimalComponentList, InvalidDynamicConfigParam) {
  constexpr std::string_view kConfigVarsTemplate = R"(
    dynamic-config-default-overrides:
      USERVER_LOG_DYNAMIC_DEBUG:
        force-disabled: []
        force-enabled: 42  # <== error
    server-port: {0}
    default-logger-path: {1}
  )";
  const auto logs_path = GetTempRoot() + "/log.txt";

  fs::blocking::RewriteFileContents(
      GetConfigVarsPath(),
      fmt::format(kConfigVarsTemplate, GetServerPort(), logs_path));

  // This is a golden test that shows how exactly dynamic config parsing failure
  // may look. Feel free to change this test if those messages ever change.
  const auto expected_exception_message = fmt::format(
      "Cannot start component dynamic-config: {} "
      "while parsing dynamic config values. Error at path "
      "'USERVER_LOG_DYNAMIC_DEBUG.force-enabled': Wrong type. "
      "Expected: arrayValue, actual: intValue",
      compiler::GetTypeName<formats::json::TypeMismatchException>());

  UEXPECT_THROW_MSG(
      components::RunOnce(components::InMemoryConfig{GetStaticConfig()},
                          components::MinimalServerComponentList()),
      std::exception, expected_exception_message);

  EXPECT_THAT(
      fs::blocking::ReadFileContents(logs_path),
      testing::HasSubstr("text=Loading failed: " + expected_exception_message));
}

USERVER_NAMESPACE_END
