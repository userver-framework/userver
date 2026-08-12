#include <gmock/gmock.h>

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/minimal_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/logging/log.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/resource_scopes.hpp>

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

TEST(ResourceScopeStorage, Smoke)
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
            context.Scopes().Register([] {
                init_called = true;
                return utils::FastScopeGuard([]() noexcept { destroy_called = true; });
            });
        }
    };

    auto component_list = components::MinimalComponentList().Append<ComponentWithResource>("component");
    components::RunOnce(components::InMemoryConfig{kConfig}, component_list);

    EXPECT_TRUE(init_called);
    EXPECT_TRUE(destroy_called);
}

TEST(ResourceScopeStorage, HappyPathOrder)
{
    /// [ResourceScopeStorage - HappyPathOrder]
    static std::vector<int> trace;

    // Reset static variables for --gtest_repeat.
    trace = {};

    class ComponentWithResource final : public components::ComponentBase {
    public:
        ComponentWithResource(const components::ComponentConfig& config, const components::ComponentContext& context)
            : components::ComponentBase(config, context)
        {
            trace.push_back(0);

            context.Scopes().Register([] {
                trace.push_back(1);
                return utils::FastScopeGuard([]() noexcept { trace.push_back(2); });
            });

            context.Scopes().Register([] {
                trace.push_back(3);
                return utils::FastScopeGuard([]() noexcept { trace.push_back(4); });
            });
        }
    };

    auto component_list = components::MinimalComponentList().Append<ComponentWithResource>("component");
    components::RunOnce(components::InMemoryConfig{kConfig}, component_list);

    EXPECT_THAT(trace, ::testing::ElementsAre(0, 1, 3, 4, 2));
    /// [ResourceScopeStorage - HappyPathOrder]
}

TEST(ResourceScopeStorage, CtrThrow)
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

            context.Scopes().Register([] {
                trace.push_back(1);
                return utils::FastScopeGuard([]() noexcept { trace.push_back(2); });
            });

            context.Scopes().Register([] {
                trace.push_back(3);
                return utils::FastScopeGuard([]() noexcept { trace.push_back(4); });
            });

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

TEST(ResourceScopeStorage, CallbackThrow)
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

            context.Scopes().Register([] {
                trace.push_back(1);
                return utils::FastScopeGuard([]() noexcept { trace.push_back(2); });
            });

            context.Scopes().Register([] {
                trace.push_back(3);
                throw std::runtime_error("1");
                return utils::FastScopeGuard([]() noexcept { trace.push_back(4); });
            });
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

TEST(ResourceScopeStorage, WithResourceScopes) {
    static std::string data_on_construction;
    static std::string data_on_destruction;

    // Reset static variables for --gtest_repeat.
    data_on_construction = {};
    data_on_destruction = {};

    class Client final {
    public:
        Client(utils::ResourceScopeStorage& resource_scope_storage, std::unique_ptr<std::string> data) {
            resource_scope_storage.Register([this] {
                data_on_construction = *data_;
                return utils::FastScopeGuard([this]() noexcept { data_on_destruction = *data_; });
            });

            // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
            data_ = std::move(data);
        }

        ~Client() { data_ = nullptr; }

    private:
        std::unique_ptr<std::string> data_;
    };

    {
        utils::WithResourceScopes<Client> client(std::in_place, std::make_unique<std::string>("data"));
    }
    EXPECT_EQ(data_on_construction, "data");
    EXPECT_EQ(data_on_destruction, "data");
}

USERVER_NAMESPACE_END
