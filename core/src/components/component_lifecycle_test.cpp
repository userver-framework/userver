#include <gtest/gtest.h>

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/minimal_component_list.hpp>
#include <userver/components/run.hpp>

#include <components/component_list_test.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

std::atomic<bool> good_component_constructed{false};
std::atomic<bool> good_component_on_all_components_loaded_called{false};
std::atomic<bool> good_component_on_all_components_stopping_called{false};
std::atomic<bool> bad_component_construction_started{false};

class GoodComponent final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "good-component";

    GoodComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
        : ComponentBase(config, context)
    {
        good_component_constructed = true;
    }

    void OnAllComponentsLoaded() override { good_component_on_all_components_loaded_called = true; }

    void OnAllComponentsAreStopping() override { good_component_on_all_components_stopping_called = true; }
};

class BadComponent final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "bad-component";

    BadComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
        : ComponentBase(config, context)
    {
        [[maybe_unused]] auto& good_component = context.FindComponent<GoodComponent>();
        bad_component_construction_started = true;
        throw std::runtime_error("BadComponent constructor intentionally throws");
    }
};

}  // namespace

TEST(ComponentLifecycle, OnAllComponentsAreStoppingCalledWhenConstructorThrows) {
    // Reset static variables for --gtest_repeat.
    good_component_constructed = false;
    good_component_on_all_components_loaded_called = false;
    good_component_on_all_components_stopping_called = false;
    bad_component_construction_started = false;

    const std::string config = tests::MergeYaml(
        tests::kMinimalStaticConfig,
        R"(
components_manager:
    components:
        logging:
            fs-task-processor: main-task-processor
            loggers:
                default:
                    file_path: '@null'
                    format: ltsv
        good-component: {}
        bad-component: {}
)"
    );

    UEXPECT_THROW_MSG(
        components::RunOnce(
            components::InMemoryConfig{config},
            components::MinimalComponentList().Append<GoodComponent>().Append<BadComponent>()
        ),
        std::runtime_error,
        "BadComponent constructor intentionally throws"
    );

    EXPECT_TRUE(good_component_constructed);
    EXPECT_TRUE(bad_component_construction_started);
    EXPECT_FALSE(good_component_on_all_components_loaded_called);
    EXPECT_TRUE(good_component_on_all_components_stopping_called);
}

USERVER_NAMESPACE_END
