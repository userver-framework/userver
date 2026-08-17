#include <gmock/gmock.h>

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/minimal_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/resource_scopes.hpp>

#include <components/component_list_test.hpp>

USERVER_NAMESPACE_BEGIN

using ResourceScopeStorage = ComponentList;

namespace {

constexpr std::string_view kConfig = R"(
components_manager:
    components:
        component: {}
)";

constexpr std::string_view kConfigWithDependency = R"(
components_manager:
    components:
        dependency: {}
)";

}  // namespace

TEST_F(ResourceScopeStorage, Smoke)
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
    components::RunOnce(
        components::InMemoryConfig{tests::MergeYaml(tests::kMinimalStaticConfig, kConfig)},
        component_list
    );

    EXPECT_TRUE(init_called);
    EXPECT_TRUE(destroy_called);
}

TEST_F(ResourceScopeStorage, HappyPathOrder)
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
    components::RunOnce(
        components::InMemoryConfig{tests::MergeYaml(tests::kMinimalStaticConfig, kConfig)},
        component_list
    );

    EXPECT_THAT(trace, ::testing::ElementsAre(0, 1, 3, 4, 2));
    /// [ResourceScopeStorage - HappyPathOrder]
}

TEST_F(ResourceScopeStorage, CtrThrow)
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
        components::RunOnce(
            components::InMemoryConfig{tests::MergeYaml(tests::kMinimalStaticConfig, kConfig)},
            component_list
        ),
        std::runtime_error,
        "1"
    );

    EXPECT_THAT(trace, ::testing::ElementsAre(0));
}

TEST_F(ResourceScopeStorage, CtrThrowDestroysScopesBeforeDependencies)
{
    static std::vector<int> trace;

    // Reset static variables for --gtest_repeat.
    trace = {};

    class Dependency final : public components::ComponentBase {
    public:
        Dependency(const components::ComponentConfig& config, const components::ComponentContext& context)
            : components::ComponentBase(config, context)
        {}

        ~Dependency() override { trace.push_back(2); }
    };

    class ComponentWithResource final : public components::ComponentBase {
    public:
        ComponentWithResource(const components::ComponentConfig& config, const components::ComponentContext& context)
            : components::ComponentBase(config, context)
        {
            trace.push_back(0);
            context.FindComponent<Dependency>("dependency");

            // AfterConstruction is not invoked if the constructor throws.
            // This test checks the destruction order of the callback object itself.
            context.Scopes().Register([guard = utils::FastScopeGuard([]() noexcept { trace.push_back(1); })] {
                trace.push_back(3);
                return utils::FastScopeGuard([]() noexcept { trace.push_back(4); });
            });

            throw std::runtime_error("1");
        }
    };

    auto component_list =
        components::MinimalComponentList().Append<Dependency>("dependency").Append<ComponentWithResource>("component");
    UEXPECT_THROW_MSG(
        components::RunOnce(
            components::InMemoryConfig{
                tests::MergeYaml(tests::MergeYaml(tests::kMinimalStaticConfig, kConfig), kConfigWithDependency)
            },
            component_list
        ),
        std::runtime_error,
        "1"
    );

    EXPECT_THAT(trace, ::testing::ElementsAre(0, 1, 2));
}

TEST_F(ResourceScopeStorage, CallbackThrow)
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

        ~ComponentWithResource() override { trace.push_back(5); }
    };

    auto component_list = components::MinimalComponentList().Append<ComponentWithResource>("component");
    UEXPECT_THROW_MSG(
        components::RunOnce(
            components::InMemoryConfig{tests::MergeYaml(tests::kMinimalStaticConfig, kConfig)},
            component_list
        ),
        std::runtime_error,
        "1"
    );

    EXPECT_THAT(trace, ::testing::ElementsAre(0, 1, 3, 2, 5));
}

TEST_F(ResourceScopeStorage, WithResourceScopes) {
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
