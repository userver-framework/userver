#include <optional>
#include <stdexcept>
#include <string_view>

#include <components/component_list_test.hpp>
#include <userver/components/component_base.hpp>
#include <userver/components/component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/logging/component.hpp>
#include <userver/logging/level.hpp>
#include <userver/logging/log.hpp>
#include <userver/os_signals/component.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kStaticConfig = R"(
components_manager:
  coro_pool:
    initial_size: 5
    max_size: 50
    stack_usage_monitor_enabled: false
  default_task_processor: main-task-processor
  fs_task_processor: main-task-processor
  event_thread_pool:
    threads: 1
  task_processors:
    main-task-processor:
      worker_threads: 1
  components:
    logging:
      fs-task-processor: main-task-processor
      loggers: {}
    span-level-observer: {}
)";

constexpr std::string_view kInfoSpanWarningLoggerConfig = R"(
components_manager:
  components:
    logging:
      span-log-level: info
      loggers:
        default:
          file_path: '@null'
          level: warning
)";

template <bool kHasSpanLevelOverride, bool kHasWarningLogger>
class SpanLevelObserver final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "span-level-observer";

    SpanLevelObserver(const components::ComponentConfig& config, const components::ComponentContext& context)
        : components::ComponentBase(config, context) {
        const auto expected_level = kHasSpanLevelOverride ? std::optional{logging::Level::kInfo} : std::nullopt;
        EXPECT_EQ(tracing::GetSpanLogLevel(), expected_level);
        const auto span = tracing::Span::MakeRootSpan("span-level-in-component-constructor");
        EXPECT_EQ(span.ShouldLogDefault(), kHasSpanLevelOverride);
        if constexpr (kHasWarningLogger) {
            EXPECT_FALSE(logging::ShouldLog(logging::Level::kInfo));
        }
    }
};

template <bool kHasSpanLevelOverride, bool kHasWarningLogger = true>
void RunWithConfig(std::string_view patch) {
    components::RunOnce(
        components::InMemoryConfig{tests::MergeYaml(kStaticConfig, patch)},
        components::ComponentList()
            .Append<os_signals::ProcessorComponent>()
            .Append<components::StatisticsStorage>()
            .Append<components::Logging>()
            .Append<SpanLevelObserver<kHasSpanLevelOverride, kHasWarningLogger>>()
    );
}

class LoggingSpanLevel : public ComponentList {
    tracing::TracingStateGuard tracing_state_guard_;
};

TEST_F(LoggingSpanLevel, AppliedBeforeComponentConstructorWithoutTracingManager) {
    RunWithConfig<true>(kInfoSpanWarningLoggerConfig);
}

TEST_F(LoggingSpanLevel, AppliedWithoutLoggers) {
    RunWithConfig<true, false>(R"(
components_manager:
  components:
    logging:
      span-log-level: info
)");
}

TEST_F(LoggingSpanLevel, MissingOptionResetsPreviousConfiguration) {
    RunWithConfig<true>(kInfoSpanWarningLoggerConfig);
    RunWithConfig<false>(R"(
components_manager:
  components:
    logging:
      loggers:
        default:
          file_path: '@null'
          level: warning
)");
}

TEST_F(LoggingSpanLevel, InvalidLevelRejected) {
    UEXPECT_THROW_MSG(
        RunWithConfig<false>(R"(
components_manager:
  components:
    logging:
      span-log-level: invalid
)"),
        std::runtime_error,
        "Unknown log level 'invalid' (must be one of 'trace', 'debug', 'info', 'warning', 'error', 'critical', 'none')"
    );
}

}  // namespace

USERVER_NAMESPACE_END
