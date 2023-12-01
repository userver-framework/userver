#include <userver/logging/component.hpp>

#include <fmt/format.h>
#include <gmock/gmock.h>

#include <components/component_list_test.hpp>
#include <userver/alerts/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/component_list.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/components/run.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/logging/log.hpp>
#include <userver/os_signals/component.hpp>
#include <userver/tracing/component.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kNoLoggingConfig = R"(
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
  components: {}
)";

components::ComponentList MakeNoLoggingComponentList() {
  return components::ComponentList()
      // Pretty much the only component that does not require Logging.
      .Append<os_signals::ProcessorComponent>();
}

}  // namespace

TEST_F(ComponentList, NoLogging) {
  UEXPECT_THROW_MSG(
      components::RunOnce(
          components::InMemoryConfig{std::string{kNoLoggingConfig}},
          MakeNoLoggingComponentList()),
      std::exception,
      "Error while parsing configs from in-memory config. Details: No "
      "component config found for 'logging', which is a required component");
}

namespace {

constexpr std::string_view kNoDefaultLoggerConfig = R"(
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
        some-logger:
          file_path: '@stderr'
          format: tskv
    tracer:
      service-name: test-service
)";

components::ComponentList MakeNoDefaultLoggerComponentList() {
  return components::ComponentList()
      .Append<os_signals::ProcessorComponent>()
      .Append<components::Logging>()
      .Append<components::Tracer>()
      .Append<alerts::StorageComponent>();
}

}  // namespace

TEST_F(ComponentList, NoDefaultLogger) {
  UEXPECT_THROW_MSG(
      components::RunOnce(
          components::InMemoryConfig{std::string{kNoDefaultLoggerConfig}},
          MakeNoDefaultLoggerComponentList()),
      std::exception,
      "Error while parsing configs from in-memory config. Details: Field "
      "'components_manager.components.logging.loggers.default.file_path' is "
      "missing");
}

namespace {

class TwoLoggersComponent final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "two-loggers";

  TwoLoggersComponent(const components::ComponentConfig& config,
                      const components::ComponentContext& context)
      : components::LoggableComponentBase(config, context),
        custom_logger_(
            context.FindComponent<components::Logging>().GetLogger("custom")) {
    LOG_INFO() << "constructor default";
    LOG_INFO_TO(custom_logger_) << "constructor custom";
  }

  ~TwoLoggersComponent() override {
    LOG_INFO() << "destructor default";
    LOG_INFO_TO(custom_logger_) << "destructor custom";
  }

 private:
  logging::LoggerPtr custom_logger_;
};

constexpr std::string_view kTwoLoggersConfig = R"(
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
          file_path: $default-logger-path
          format: tskv
        custom:
          file_path: $custom-logger-path
          format: tskv
    tracer:
      service-name: test-service
    two-loggers:
)";

constexpr std::string_view kTwoLoggersConfigVars = R"(
default-logger-path: {0}
custom-logger-path: {1}
)";

components::ComponentList MakeTwoLoggersComponentList() {
  return components::ComponentList()
      .Append<os_signals::ProcessorComponent>()
      .Append<components::Logging>()
      .Append<components::Tracer>()
      .Append<TwoLoggersComponent>()
      .Append<alerts::StorageComponent>();
}

}  // namespace

TEST_F(ComponentList, CustomLogger) {
  const auto temp_root = fs::blocking::TempDirectory::Create();
  const auto default_logger_path = temp_root.GetPath() + "/log.txt";
  const auto custom_logger_path = temp_root.GetPath() + "/custom_log.txt";
  const auto config_path = temp_root.GetPath() + "/config.yaml";
  const auto config_vars_path = temp_root.GetPath() + "/config_vars.yaml";

  fs::blocking::RewriteFileContents(config_path, kTwoLoggersConfig);

  fs::blocking::RewriteFileContents(
      config_vars_path, fmt::format(kTwoLoggersConfigVars, default_logger_path,
                                    custom_logger_path));

  UASSERT_NO_THROW(components::RunOnce(config_path, config_vars_path,
                                       std::nullopt,
                                       MakeTwoLoggersComponentList()));

  const auto default_log = fs::blocking::ReadFileContents(default_logger_path);
  const auto custom_log = fs::blocking::ReadFileContents(custom_logger_path);

  EXPECT_THAT(default_log, testing::HasSubstr(fmt::format(
                               "Parsed configs from file '{}' "
                               "using config_vars from cmdline in file '{}'",
                               config_path, config_vars_path)));

  EXPECT_THAT(default_log, testing::HasSubstr("constructor default"));
  EXPECT_THAT(default_log, testing::HasSubstr("destructor default"));

  EXPECT_THAT(custom_log, testing::HasSubstr("constructor custom"));
  EXPECT_THAT(custom_log, testing::HasSubstr("destructor custom"));
}

USERVER_NAMESPACE_END
