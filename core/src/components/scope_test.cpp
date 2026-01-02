#include <gmock/gmock.h>

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/minimal_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/components/scope.hpp>
#include <userver/logging/log.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const std::string kConfig = R"(
components_manager:
  coro_pool:
    initial_size: 50
    max_size: 500
  default_task_processor: main-task-processor
  fs_task_processor: main-task-processor
  event_thread_pool:
    threads: 1
  task_processors:
    main-task-processor:
      worker_threads: 1
  components:
    component: {}
    logging:
      loggers: {}
)";

}  // namespace

TEST(ComponentsScope, Smoke)
{
    static bool init_called{};
    static bool destroy_called{};

    // Reset static variables for --gtest_repeat.
    init_called = false;
    destroy_called = false;

    class ComponentWithResource final : public components::ComponentBase {
    public:
        ComponentWithResource(const components::ComponentConfig& config, const components::ComponentContext& context)
            : components::ComponentBase(config, context)
        {
            context.RegisterScope(components::MakeScope([] {
                init_called = true;
                return utils::FastScopeGuard([]() noexcept { destroy_called = true; });
            }));
        }
    };

    auto component_list = components::MinimalComponentList().Append<ComponentWithResource>("component");
    components::RunOnce(components::InMemoryConfig{kConfig}, component_list);

    EXPECT_TRUE(init_called);
    EXPECT_TRUE(destroy_called);
}

TEST(ComponentsScope, HappyPathOrder)
{
    static std::vector<int> trace;

    // Reset static variables for --gtest_repeat.
    trace = {};

    class ComponentWithResource final : public components::ComponentBase {
    public:
        ComponentWithResource(const components::ComponentConfig& config, const components::ComponentContext& context)
            : components::ComponentBase(config, context)
        {
            trace.push_back(0);

            context.RegisterScope(components::MakeScope([] {
                trace.push_back(1);
                return utils::FastScopeGuard([]() noexcept { trace.push_back(2); });
            }));

            context.RegisterScope(components::MakeScope([] {
                trace.push_back(3);
                return utils::FastScopeGuard([]() noexcept { trace.push_back(4); });
            }));
        }
    };

    auto component_list = components::MinimalComponentList().Append<ComponentWithResource>("component");
    components::RunOnce(components::InMemoryConfig{kConfig}, component_list);

    EXPECT_THAT(trace, ::testing::ElementsAre(0, 1, 3, 4, 2));
}

TEST(ComponentsScope, CtrThrow)
{
    static std::vector<int> trace;

    // Reset static variables for --gtest_repeat.
    trace = {};

    class ComponentWithResource final : public components::ComponentBase {
    public:
        ComponentWithResource(const components::ComponentConfig& config, const components::ComponentContext& context)
            : components::ComponentBase(config, context)
        {
            trace.push_back(0);

            context.RegisterScope(components::MakeScope([] {
                trace.push_back(1);
                return utils::FastScopeGuard([]() noexcept { trace.push_back(2); });
            }));

            context.RegisterScope(components::MakeScope([] {
                trace.push_back(3);
                return utils::FastScopeGuard([]() noexcept { trace.push_back(4); });
            }));

            throw std::runtime_error("1");
        }
    };

    auto component_list = components::MinimalComponentList().Append<ComponentWithResource>("component");
    UEXPECT_THROW_MSG(
        components::RunOnce(components::InMemoryConfig{kConfig}, component_list),
        std::runtime_error,
        "1"
    );

    EXPECT_THAT(trace, ::testing::ElementsAre(0));
}

TEST(ComponentsScope, CallbackThrow)
{
    static std::vector<int> trace;

    // Reset static variables for --gtest_repeat.
    trace = {};

    class ComponentWithResource final : public components::ComponentBase {
    public:
        ComponentWithResource(const components::ComponentConfig& config, const components::ComponentContext& context)
            : components::ComponentBase(config, context)
        {
            trace.push_back(0);

            context.RegisterScope(components::MakeScope([] {
                trace.push_back(1);
                return utils::FastScopeGuard([]() noexcept { trace.push_back(2); });
            }));

            context.RegisterScope(components::MakeScope([] {
                trace.push_back(3);
                throw std::runtime_error("1");
                return utils::FastScopeGuard([]() noexcept { trace.push_back(4); });
            }));
        }
    };

    auto component_list = components::MinimalComponentList().Append<ComponentWithResource>("component");
    UEXPECT_THROW_MSG(
        components::RunOnce(components::InMemoryConfig{kConfig}, component_list),
        std::runtime_error,
        "1"
    );

    EXPECT_THAT(trace, ::testing::ElementsAre(0, 1, 3, 2));
}

USERVER_NAMESPACE_END
