#include <userver/components/minimal_server_component_list.hpp>

#include <fmt/format.h>
#include <gmock/gmock.h>

#include <userver/components/loggable_component_base.hpp>
#include <userver/components/run.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/temp_directory.hpp>  // for fs::blocking::TempDirectory
#include <userver/fs/blocking/write.hpp>  // for fs::blocking::RewriteFileContents
#include <userver/logging/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <components/component_list_test.hpp>
#include <userver/internal/net/net_listener.hpp>
#include <userver/utest/current_process_open_files.hpp>
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
      thread_name: fs-worker
      worker_threads: 2
    main-task-processor:
      thread_name: main-worker
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
          file_path: '@null'
          level: warning
    dynamic-config:
      fs-cache-path: $runtime_config_path
      fs-task-processor: main-task-processor
    dynamic-config-fallbacks:
        fallback-path: $runtime_config_path
    server:
      listener:
          port: $server-port
          task_processor: main-task-processor
    statistics-storage: # Nothing
    auth-checker-settings: # Nothing
    manager-controller:  # Nothing

    init-open-file-checker:
        init-logger-path: $init_log_path
        init-logger-path#fallback: ''
config_vars: )";

class ServerMinimalComponentList : public ComponentList {
 protected:
  const std::string& GetTempRoot() const { return temp_root_.GetPath(); }

  std::string GetRuntimeConfigPath() const {
    return temp_root_.GetPath() + "/runtime_config.json";
  }

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

class InitOpenFileChecker final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "init-open-file-checker";

  InitOpenFileChecker(const components::ComponentConfig& conf,
                      const components::ComponentContext& ctx)
      : LoggableComponentBase(conf, ctx) {
    init_path_ = conf["init-logger-path"].As<std::string>();
    if (init_path_.empty()) {
      return;
    }
  }

  void OnAllComponentsAreStopping() override {
    if (init_path_.empty()) {
      return;
    }

    const auto files = utest::CurrentProcessOpenFiles();
    ASSERT_THAT(files, testing::Not(testing::Contains(init_path_)))
        << "Initial log file should be closed after the component system "
           "started. Otherwise the open file descriptor prevents log file "
           "deletion/rotation";

    const auto logs = fs::blocking::ReadFileContents(init_path_);
    EXPECT_THAT(logs, testing::HasSubstr("Using config_vars from config.yaml."))
        << "Initial logs were not written to the init log. Init log content: "
        << logs;
  }

  static void AssertFilesWereChecked() {
    ASSERT_FALSE(init_path_.empty());
    init_path_ = {};
  }

  static yaml_config::Schema GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: Sigusr1Checker component
additionalProperties: false
properties:
    init-logger-path:
        type: string
        description: init logger path
)");
  }

 private:
  inline static std::string init_path_{};
};

auto TestsComponentList() {
  return components::MinimalServerComponentList().Append<InitOpenFileChecker>();
}

}  // namespace

TEST_F(ServerMinimalComponentList, Basic) {
  constexpr std::string_view kConfigVarsTemplate = R"(
    runtime_config_path: {0}
    server-port: {1}
  )";
  const auto config_vars =
      fmt::format(kConfigVarsTemplate, GetRuntimeConfigPath(), GetServerPort());

  fs::blocking::RewriteFileContents(GetRuntimeConfigPath(),
                                    tests::GetRuntimeConfig());
  fs::blocking::RewriteFileContents(GetConfigVarsPath(), config_vars);

  components::RunOnce(components::InMemoryConfig{GetStaticConfig()},
                      TestsComponentList());
}

TEST_F(ServerMinimalComponentList, InitLogsClose) {
  constexpr std::string_view kConfigVarsTemplate = R"(
    runtime_config_path: {0}
    init_log_path: {1}
    server-port: {2}
  )";
  const std::string init_logs_path = GetTempRoot() + "/init_log.txt";
  const auto config_vars =
      fmt::format(kConfigVarsTemplate, GetRuntimeConfigPath(), init_logs_path,
                  GetServerPort());

  fs::blocking::RewriteFileContents(GetRuntimeConfigPath(),
                                    tests::GetRuntimeConfig());
  fs::blocking::RewriteFileContents(GetConfigVarsPath(), config_vars);

  components::RunOnce(components::InMemoryConfig{GetStaticConfig()},
                      TestsComponentList());

  InitOpenFileChecker::AssertFilesWereChecked();
}

TEST_F(ServerMinimalComponentList, TraceSwitching) {
  constexpr std::string_view kConfigVarsTemplate = R"(
    runtime_config_path: {0}
    tracer_log_path: {1}
    server-port: {2}
  )";
  const std::string logs_path = GetTempRoot() + "/tracing_log.txt";
  const auto config_vars = fmt::format(
      kConfigVarsTemplate, GetRuntimeConfigPath(), logs_path, GetServerPort());

  fs::blocking::RewriteFileContents(GetRuntimeConfigPath(),
                                    tests::GetRuntimeConfig());
  fs::blocking::RewriteFileContents(GetConfigVarsPath(), config_vars);

  components::RunOnce(components::InMemoryConfig{GetStaticConfig()},
                      TestsComponentList());

  logging::LogFlush();

  const auto logs = fs::blocking::ReadFileContents(logs_path);
  EXPECT_NE(logs.find(" changed state to kQueued"), std::string::npos);
  EXPECT_NE(logs.find(" changed state to kRunning"), std::string::npos);
  EXPECT_NE(logs.find(" changed state to kCompleted"), std::string::npos);
  EXPECT_EQ(logs.find("stacktrace= 0# "), std::string::npos);
}

TEST_F(ServerMinimalComponentList, TraceStacktraces) {
  constexpr std::string_view kConfigVarsTemplate = R"(
    runtime_config_path: {0}
    tracer_log_path: {1}
    tracer_level: debug
    server-port: {2}
  )";
  const std::string logs_path = GetTempRoot() + "/tracing_st_log.txt";

  fs::blocking::RewriteFileContents(GetRuntimeConfigPath(),
                                    tests::GetRuntimeConfig());
  fs::blocking::RewriteFileContents(
      GetConfigVarsPath(),
      fmt::format(kConfigVarsTemplate, GetRuntimeConfigPath(), logs_path,
                  GetServerPort()));

  components::RunOnce(components::InMemoryConfig{GetStaticConfig()},
                      TestsComponentList());

  logging::LogFlush();

  const auto logs = fs::blocking::ReadFileContents(logs_path);
  EXPECT_NE(logs.find(" changed state to kQueued"), std::string::npos);
  EXPECT_NE(logs.find(" changed state to kRunning"), std::string::npos);
  EXPECT_NE(logs.find(" changed state to kCompleted"), std::string::npos);
  EXPECT_NE(logs.find("stacktrace= 0# "), std::string::npos);
}

TEST_F(ServerMinimalComponentList, MissingRuntimeConfigParam) {
  constexpr std::string_view kConfigVarsTemplate = R"(
    runtime_config_path: {0}
    server-port: {1}
  )";
  const auto config_vars =
      fmt::format(kConfigVarsTemplate, GetRuntimeConfigPath(), GetServerPort());

  formats::json::ValueBuilder runtime_config{
      formats::json::FromString(tests::GetRuntimeConfig())};
  runtime_config.Remove("USERVER_LOG_REQUEST_HEADERS");
  const auto runtime_config_missing_param =
      formats::json::ToString(runtime_config.ExtractValue());

  fs::blocking::RewriteFileContents(GetRuntimeConfigPath(),
                                    runtime_config_missing_param);
  fs::blocking::RewriteFileContents(GetConfigVarsPath(), config_vars);

  UEXPECT_THROW_MSG(
      components::RunOnce(components::InMemoryConfig{GetStaticConfig()},
                          TestsComponentList()),
      std::exception, "USERVER_LOG_REQUEST_HEADERS");
}

USERVER_NAMESPACE_END
