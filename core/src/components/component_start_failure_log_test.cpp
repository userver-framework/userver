#include <algorithm>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/engine/exception.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/logging/component.hpp>
#include <userver/logging/log.hpp>
#include <userver/os_signals/component.hpp>
#include <userver/utest/log_capture_fixture.hpp>
#include <userver/utest/utest.hpp>

#include <components/component_list_test.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

// Sleeps in its constructor until the sibling component fails and cancels component loading.
class SlowStartingComponent final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "slow-component";

    SlowStartingComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
        : ComponentBase(config, context)
    {
        engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
        if (engine::current_task::ShouldCancel()) {
            throw engine::WaitInterruptedException(engine::current_task::CancellationReason());
        }
    }
};

// Waits for SlowStartingComponent via FindComponent. After CancelComponentsLoad it gets
// ComponentsLoadCancelledException; that failure must be logged as WARNING (not the root cause).
class DependentOnSlowComponent final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "dependent-on-slow-component";

    DependentOnSlowComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
        : ComponentBase(config, context)
    {
        [[maybe_unused]] auto& slow = context.FindComponent<SlowStartingComponent>();
    }
};

// Yields several times so the slow / dependent components start waiting, then throws.
class FailingComponent final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "failing-component";

    FailingComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
        : ComponentBase(config, context)
    {
        for (int i = 0; i < 10; ++i) {
            engine::Yield();
        }
        throw std::runtime_error("FailingComponent constructor intentionally throws");
    }
};

constexpr std::string_view kCannotStartComponentLogTextPrefix = "Cannot start component";
constexpr std::string_view kOnLoadingCancelledLogTextPrefix = "Call OnLoadingCancelled() for component";

std::size_t CountCannotStartComponentLogs(
    const std::vector<utest::LogRecord>& records,
    std::optional<std::string_view> level = std::nullopt,
    std::optional<std::string_view> component_name = std::nullopt
) {
    return std::ranges::count_if(records, [&](const utest::LogRecord& record) {
        return record.GetText().starts_with(kCannotStartComponentLogTextPrefix) &&
               (!level || record.GetTagOptional("level") == *level) &&
               (!component_name || record.GetTagOptional("component_name") == *component_name);
    });
}

std::size_t CountOnLoadingCancelledLogs(
    const std::vector<utest::LogRecord>& records,
    std::optional<std::string_view> level = std::nullopt,
    std::optional<std::string_view> component_name = std::nullopt
) {
    return std::ranges::count_if(records, [&](const utest::LogRecord& record) {
        return record.GetText().starts_with(kOnLoadingCancelledLogTextPrefix) &&
               !record.GetTagOptional("component_name") && (!level || record.GetTagOptional("level") == *level) &&
               (!component_name ||
                record.GetText() == fmt::format("Call OnLoadingCancelled() for component '{}'", *component_name));
    });
}

}  // namespace

using ComponentStartFailureLog = ComponentList;

TEST_F(ComponentStartFailureLog, RootCauseAndCancelledComponentLogs) {
    const auto temp_root = fs::blocking::TempDirectory::Create();
    const std::string logs_path = temp_root.GetPath() + "/log.txt";

    const std::string config = tests::MergeYaml(
        tests::kMinimalStaticConfig,
        fmt::format(
            R"(
            components_manager:
                components:
                    slow-component: {{}}
                    dependent-on-slow-component: {{}}
                    failing-component: {{}}
                    statistics-storage: {{}}
                    logging:
                        loggers:
                            default:
                                file_path: {}
                                format: tskv
                                level: debug
            )",
            logs_path
        )
    );

    auto component_list =
        components::ComponentList()
            .Append<os_signals::ProcessorComponent>()
            .Append<components::StatisticsStorage>()
            .Append<components::Logging>()
            .Append<SlowStartingComponent>()
            .Append<DependentOnSlowComponent>()
            .Append<FailingComponent>();

    UEXPECT_THROW_MSG(
        components::RunOnce(components::InMemoryConfig{config}, component_list),
        std::runtime_error,
        "FailingComponent constructor intentionally throws"
    );

    logging::LogFlush();

    const auto records = utest::ParseTskvLogRecords(fs::blocking::ReadFileContents(logs_path));

    ASSERT_EQ(CountCannotStartComponentLogs(records, "ERROR", FailingComponent::kName), 1) << records;
    ASSERT_EQ(CountCannotStartComponentLogs(records, "WARNING", SlowStartingComponent::kName), 1) << records;
    ASSERT_EQ(CountCannotStartComponentLogs(records, "WARNING", DependentOnSlowComponent::kName), 1) << records;
    ASSERT_EQ(CountCannotStartComponentLogs(records), 3) << records;

    ASSERT_EQ(CountOnLoadingCancelledLogs(records, "DEBUG", SlowStartingComponent::kName), 1) << records;
    ASSERT_EQ(CountOnLoadingCancelledLogs(records, "DEBUG", DependentOnSlowComponent::kName), 1) << records;
    ASSERT_EQ(CountOnLoadingCancelledLogs(records, "DEBUG", FailingComponent::kName), 1) << records;
    ASSERT_EQ(CountOnLoadingCancelledLogs(records, "DEBUG"), std::ranges::distance(component_list)) << records;
}

USERVER_NAMESPACE_END
