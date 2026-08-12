#include <gtest/gtest.h>

#include <atomic>
#include <chrono>

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/minimal_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/dynamic_config/updates_sink/component.hpp>
#include <userver/dynamic_config/updates_sink/find.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utest/utest.hpp>

#include <components/component_list_test.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kUpdaterFailureError = "simulated updater failure";

std::atomic<bool> slow_component_interrupted{false};

// Registered first: blocks in its constructor for far longer than the test is
// allowed to run. It must be interrupted by the cancellation triggered once the
// sibling component throws.
class SlowComponent final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "slow-component";

    SlowComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
        : ComponentBase(config, context)
    {
        engine::InterruptibleSleepFor(2 * utest::kMaxTestWaitTime);
        if (engine::current_task::ShouldCancel()) {
            slow_component_interrupted = true;
        }
    }
};

// Registered second: throws from its constructor, which must cancel the load and
// interrupt the SlowComponent constructor.
class ThrowingComponent final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "throwing-component";

    ThrowingComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
        : ComponentBase(config, context)
    {
        throw std::runtime_error("ThrowingComponent constructor intentionally throws");
    }
};

// Mimics a dynamic-config updater that fails to fetch the initial config:
// NotifyLoadingFailed wakes components waiting on DynamicConfig, then the
// updater itself throws only ComponentsLoadCancelledException. Other component
// start tasks must still be cancelled promptly.
class FailingUpdaterComponent final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "failing-updater";

    FailingUpdaterComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
        : ComponentBase(config, context)
    {
        // Let SlowComponent enter its interruptible sleep before we fail.
        for (int i = 0; i < 10; ++i) {
            engine::Yield();
        }

        auto& updates_sink = dynamic_config::FindUpdatesSink(config, context);
        updates_sink.NotifyLoadingFailed(kName, kUpdaterFailureError);
        throw components::ComponentsLoadCancelledException(kUpdaterFailureError);
    }
};

}  // namespace

using ComponentConstructorCancel = ComponentList;

TEST_F(ComponentConstructorCancel, ThrowingComponentInterruptsSlowConstructor) {
    // Reset static variables for --gtest_repeat.
    slow_component_interrupted = false;

    const std::string config = tests::MergeYaml(
        tests::kMinimalStaticConfig,
        R"(
        components_manager:
            components:
                slow-component: {}
                throwing-component: {}
        )"
    );

    const auto start = std::chrono::steady_clock::now();

    UEXPECT_THROW_MSG(
        components::RunOnce(
            components::InMemoryConfig{config},
            components::MinimalComponentList().Append<SlowComponent>().Append<ThrowingComponent>()
        ),
        std::runtime_error,
        "ThrowingComponent constructor intentionally throws"
    );

    const auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_LT(elapsed, utest::kMaxTestWaitTime)
        << "Component load did not abort promptly after a constructor threw; the "
           "SlowComponent constructor was likely not interrupted";
    EXPECT_TRUE(slow_component_interrupted);
}

TEST_F(ComponentConstructorCancel, ComponentsLoadCancelledExceptionInterruptsSlowConstructor) {
    // Reset static variables for --gtest_repeat.
    slow_component_interrupted = false;

    const std::string config = tests::MergeYaml(
        tests::kMinimalStaticConfig,
        R"(
        components_manager:
            components:
                dynamic-config:
                    updates-enabled: true
                slow-component: {}
                failing-updater: {}
        )"
    );

    const auto start = std::chrono::steady_clock::now();

    UEXPECT_THROW_MSG(
        components::RunOnce(
            components::InMemoryConfig{config},
            components::MinimalComponentList().Append<SlowComponent>().Append<FailingUpdaterComponent>()
        ),
        std::runtime_error,
        kUpdaterFailureError
    );

    const auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_LT(elapsed, utest::kMaxTestWaitTime)
        << "Component load did not abort promptly after only ComponentsLoadCancelledException "
           "was thrown; the SlowComponent constructor was likely not interrupted";
    EXPECT_TRUE(slow_component_interrupted);
}

USERVER_NAMESPACE_END
