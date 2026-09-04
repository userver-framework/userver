#include <gmock/gmock.h>

#include <memory>

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/minimal_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/impl/internal_tag.hpp>
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

TEST_F(ResourceScopeStorage, DependencyAfterConstructionBeforeDependentCtor)
{
    static std::vector<int> trace;

    // Reset static variables for --gtest_repeat.
    trace = {};

    class Dependency final : public components::ComponentBase {
    public:
        Dependency(const components::ComponentConfig& config, const components::ComponentContext& context)
            : components::ComponentBase(config, context)
        {
            trace.push_back(0);
            context.Scopes().Register([] {
                trace.push_back(1);
                return utils::FastScopeGuard([]() noexcept { trace.push_back(4); });
            });
        }
    };

    class Dependent final : public components::ComponentBase {
    public:
        Dependent(const components::ComponentConfig& config, const components::ComponentContext& context)
            : components::ComponentBase(config, context)
        {
            context.FindComponent<Dependency>("dependency");
            trace.push_back(2);
        }

        ~Dependent() override { trace.push_back(3); }
    };

    auto component_list =
        components::MinimalComponentList().Append<Dependency>("dependency").Append<Dependent>("component");
    components::RunOnce(
        components::InMemoryConfig{
            tests::MergeYaml(tests::MergeYaml(tests::kMinimalStaticConfig, kConfig), kConfigWithDependency)
        },
        component_list
    );

    EXPECT_THAT(trace, ::testing::ElementsAre(0, 1, 2, 3, 4));
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

TEST_F(ResourceScopeStorage, MakeWithResourceScopes) {
    static std::string data_on_construction;
    static std::string data_on_destruction;

    // Reset static variables for --gtest_repeat.
    data_on_construction = {};
    data_on_destruction = {};

    /// [MakeWithResourceScopes]
    class Client final {
    public:
        Client(utils::ResourceScopeStorage& resource_scope_storage, std::string data)
            : data_(std::make_unique<std::string>(std::move(data)))
        {
            resource_scope_storage.Register([this] {
                data_on_construction = *data_;
                return utils::FastScopeGuard([this]() noexcept { data_on_destruction = *data_; });
            });
        }

        ~Client() { data_ = nullptr; }

        const std::string& GetData() const { return *data_; }

    private:
        // We check that it does not happen that ~Client already dropped the
        // string while BeforeDestruction still reads it.
        std::unique_ptr<std::string> data_;
    };

    std::shared_ptr<Client> client = utils::MakeWithResourceScopes<Client>("data");
    EXPECT_EQ(client->GetData(), "data");
    /// [MakeWithResourceScopes]
    EXPECT_EQ(data_on_construction, "data");
    EXPECT_TRUE(data_on_destruction.empty());

    {
        auto copy = client;
        EXPECT_EQ(copy.use_count(), 2);
        EXPECT_EQ(copy.get(), client.get());
        copy.reset();
        EXPECT_EQ(client.use_count(), 1);
        EXPECT_TRUE(data_on_destruction.empty());
    }

    client.reset();
    EXPECT_EQ(data_on_destruction, "data");
}

namespace {

auto MakeTraceScope(std::vector<int>& after, std::vector<int>& before, int id) {
    return [&after, &before, id] {
        after.push_back(id);
        return utils::FastScopeGuard([&before, id]() noexcept { before.push_back(id); });
    };
}

}  // namespace

TEST(ResourceScopeStorageUnit, PriorityOrder)
{
    utils::ResourceScopeStorage scopes;
    std::vector<int> after;
    std::vector<int> before;

    scopes.Register(utils::impl::InternalTag{}, 1, MakeTraceScope(after, before, 1));
    scopes.Register(utils::impl::InternalTag{}, -1, MakeTraceScope(after, before, -1));
    scopes.Register(utils::impl::InternalTag{}, 0, MakeTraceScope(after, before, 0));

    scopes.AfterConstruction();
    EXPECT_THAT(after, ::testing::ElementsAre(-1, 0, 1));

    scopes.BeforeDestruction();
    EXPECT_THAT(before, ::testing::ElementsAre(1, 0, -1));
}

TEST(ResourceScopeStorageUnit, EqualPriorityKeepsRegistrationOrder)
{
    utils::ResourceScopeStorage scopes;
    std::vector<int> after;
    std::vector<int> before;

    scopes.Register(utils::impl::InternalTag{}, 0, MakeTraceScope(after, before, 1));
    scopes.Register(utils::impl::InternalTag{}, 0, MakeTraceScope(after, before, 2));
    scopes.Register(utils::impl::InternalTag{}, 0, MakeTraceScope(after, before, 3));

    scopes.AfterConstruction();
    EXPECT_THAT(after, ::testing::ElementsAre(1, 2, 3));

    scopes.BeforeDestruction();
    EXPECT_THAT(before, ::testing::ElementsAre(3, 2, 1));
}

TEST(ResourceScopeStorageUnit, DefaultPriorityIsZero)
{
    utils::ResourceScopeStorage scopes;
    std::vector<int> after;
    std::vector<int> before;

    scopes.Register(utils::impl::InternalTag{}, 1, MakeTraceScope(after, before, 1));
    scopes.Register(MakeTraceScope(after, before, 0));
    scopes.Register(utils::impl::InternalTag{}, -1, MakeTraceScope(after, before, -1));

    scopes.AfterConstruction();
    EXPECT_THAT(after, ::testing::ElementsAre(-1, 0, 1));

    scopes.BeforeDestruction();
    EXPECT_THAT(before, ::testing::ElementsAre(1, 0, -1));
}

TEST(ResourceScopeStorageUnit, PriorityOnUnusedFactories)
{
    utils::ResourceScopeStorage scopes;
    std::vector<int> before;

    scopes.Register(utils::impl::InternalTag{}, 1, [guard = utils::FastScopeGuard([&before]() noexcept {
                                                        before.push_back(1);
                                                    })] { return utils::FastScopeGuard([]() noexcept {}); });
    scopes.Register(utils::impl::InternalTag{}, -1, [guard = utils::FastScopeGuard([&before]() noexcept {
                                                         before.push_back(-1);
                                                     })] { return utils::FastScopeGuard([]() noexcept {}); });

    scopes.BeforeDestruction();
    EXPECT_THAT(before, ::testing::ElementsAre(1, -1));
}

TEST(ResourceScopeStorageUnit, AfterConstructionThrowRunsBeforeDestruction)
{
    utils::ResourceScopeStorage scopes;
    std::vector<int> trace;

    scopes.Register([&trace] {
        trace.push_back(1);
        return utils::FastScopeGuard([&trace]() noexcept { trace.push_back(2); });
    });
    scopes.Register([&trace] {
        trace.push_back(3);
        throw std::runtime_error("1");
        return utils::FastScopeGuard([&trace]() noexcept { trace.push_back(4); });
    });

    UEXPECT_THROW_MSG(scopes.AfterConstruction(), std::runtime_error, "1");
    EXPECT_THAT(trace, ::testing::ElementsAre(1, 3, 2));
}

TEST(ResourceScopeStorageUnit, NestedRegisterDuringAfterConstruction)
{
    utils::ResourceScopeStorage scopes;
    std::vector<int> trace;

    scopes.Register([&scopes, &trace] {
        trace.push_back(1);
        scopes.Register([&trace] {
            trace.push_back(2);
            return utils::FastScopeGuard([&trace]() noexcept { trace.push_back(3); });
        });
        return utils::FastScopeGuard([&trace]() noexcept { trace.push_back(4); });
    });

    scopes.AfterConstruction();
    EXPECT_THAT(trace, ::testing::ElementsAre(1, 2));

    scopes.BeforeDestruction();
    EXPECT_THAT(trace, ::testing::ElementsAre(1, 2, 4, 3));
}

TEST(ResourceScopeStorageUnit, NestedRegisterThrowSwallowed)
{
    utils::ResourceScopeStorage scopes;
    std::vector<int> trace;

    scopes.Register([&scopes, &trace] {
        trace.push_back(1);
        try {
            scopes.Register([&trace] {
                trace.push_back(2);
                throw std::runtime_error("nested");
                return utils::FastScopeGuard([&trace]() noexcept { trace.push_back(3); });
            });
        } catch (const std::runtime_error&) {
            trace.push_back(4);
        }
        return utils::FastScopeGuard([&trace]() noexcept { trace.push_back(5); });
    });

    UEXPECT_NO_THROW(scopes.AfterConstruction());
    EXPECT_THAT(trace, ::testing::ElementsAre(1, 2, 4));

    scopes.BeforeDestruction();
    EXPECT_THAT(trace, ::testing::ElementsAre(1, 2, 4, 5));
}

TEST(ResourceScopeStorageUnit, NestedRegisterThrowPropagates)
{
    utils::ResourceScopeStorage scopes;
    std::vector<int> trace;

    scopes.Register([&scopes, &trace] {
        trace.push_back(1);
        scopes.Register([&trace] {
            trace.push_back(2);
            throw std::runtime_error("nested");
            return utils::FastScopeGuard([&trace]() noexcept { trace.push_back(3); });
        });
        return utils::FastScopeGuard([&trace]() noexcept { trace.push_back(4); });
    });

    UEXPECT_THROW_MSG(scopes.AfterConstruction(), std::runtime_error, "nested");
    EXPECT_THAT(trace, ::testing::ElementsAre(1, 2));
}

TEST(ResourceScopeStorageUnit, NestedRegisterCleansUpOnLaterThrow)
{
    utils::ResourceScopeStorage scopes;
    std::vector<int> trace;

    scopes.Register([&scopes, &trace] {
        trace.push_back(1);
        scopes.Register([&trace] {
            trace.push_back(2);
            return utils::FastScopeGuard([&trace]() noexcept { trace.push_back(3); });
        });
        return utils::FastScopeGuard([&trace]() noexcept { trace.push_back(4); });
    });
    scopes.Register([&trace] {
        trace.push_back(5);
        throw std::runtime_error("1");
        return utils::FastScopeGuard([&trace]() noexcept { trace.push_back(6); });
    });

    UEXPECT_THROW_MSG(scopes.AfterConstruction(), std::runtime_error, "1");
    EXPECT_THAT(trace, ::testing::ElementsAre(1, 2, 5, 4, 3));
}

TEST(ResourceScopeStorageUnit, RegisterAfterReady)
{
    utils::ResourceScopeStorage scopes;
    std::vector<int> trace;

    scopes.Register([&trace] {
        trace.push_back(1);
        return utils::FastScopeGuard([&trace]() noexcept { trace.push_back(2); });
    });
    scopes.AfterConstruction();

    scopes.Register([&trace] {
        trace.push_back(3);
        return utils::FastScopeGuard([&trace]() noexcept { trace.push_back(4); });
    });
    EXPECT_THAT(trace, ::testing::ElementsAre(1, 3));

    scopes.BeforeDestruction();
    EXPECT_THAT(trace, ::testing::ElementsAre(1, 3, 4, 2));
}

TEST(ResourceScopeStorageUnit, NestedRegisterDuringReady)
{
    utils::ResourceScopeStorage scopes;
    std::vector<int> trace;

    scopes.AfterConstruction();
    scopes.Register([&scopes, &trace] {
        trace.push_back(1);
        scopes.Register([&trace] {
            trace.push_back(2);
            return utils::FastScopeGuard([&trace]() noexcept { trace.push_back(3); });
        });
        return utils::FastScopeGuard([&trace]() noexcept { trace.push_back(4); });
    });
    EXPECT_THAT(trace, ::testing::ElementsAre(1, 2));

    scopes.BeforeDestruction();
    EXPECT_THAT(trace, ::testing::ElementsAre(1, 2, 4, 3));
}

TEST(ResourceScopeStorageUnit, NestedRegisterDuringReadyThrowSwallowed)
{
    utils::ResourceScopeStorage scopes;
    std::vector<int> trace;

    scopes.AfterConstruction();
    scopes.Register([&scopes, &trace] {
        trace.push_back(1);
        try {
            scopes.Register([&trace] {
                trace.push_back(2);
                throw std::runtime_error("nested");
                return utils::FastScopeGuard([&trace]() noexcept { trace.push_back(3); });
            });
        } catch (const std::runtime_error&) {
            trace.push_back(4);
        }
        return utils::FastScopeGuard([&trace]() noexcept { trace.push_back(5); });
    });
    EXPECT_THAT(trace, ::testing::ElementsAre(1, 2, 4));

    scopes.BeforeDestruction();
    EXPECT_THAT(trace, ::testing::ElementsAre(1, 2, 4, 5));
}

TEST(ResourceScopeStorageUnit, RegisterDuringReadyThrowDoesNotCloseOthers)
{
    utils::ResourceScopeStorage scopes;
    std::vector<int> trace;

    scopes.Register([&trace] {
        trace.push_back(1);
        return utils::FastScopeGuard([&trace]() noexcept { trace.push_back(2); });
    });
    scopes.AfterConstruction();

    UEXPECT_THROW_MSG(
        scopes.Register([&trace] {
            trace.push_back(3);
            throw std::runtime_error("ready");
            return utils::FastScopeGuard([&trace]() noexcept { trace.push_back(4); });
        }),
        std::runtime_error,
        "ready"
    );
    EXPECT_THAT(trace, ::testing::ElementsAre(1, 3));

    scopes.BeforeDestruction();
    EXPECT_THAT(trace, ::testing::ElementsAre(1, 3, 2));
}

TEST(ResourceScopeStorageUnit, RegisterDuringBeforeDestruction)
{
    utils::ResourceScopeStorage scopes;
    std::vector<int> trace;

    scopes.Register([&scopes, &trace] {
        trace.push_back(1);
        return utils::FastScopeGuard([&scopes, &trace]() noexcept {
            trace.push_back(2);
            scopes.Register([&trace] {
                trace.push_back(3);
                return utils::FastScopeGuard([&trace]() noexcept { trace.push_back(4); });
            });
            trace.push_back(5);
        });
    });

    scopes.AfterConstruction();
    scopes.BeforeDestruction();
    EXPECT_THAT(trace, ::testing::ElementsAre(1, 2, 3, 4, 5));
}

TEST(ResourceScopeStorageUnit, NestedRegisterDuringBeforeDestruction)
{
    utils::ResourceScopeStorage scopes;
    std::vector<int> trace;

    scopes.Register([&scopes, &trace] {
        return utils::FastScopeGuard([&scopes, &trace]() noexcept {
            trace.push_back(1);
            scopes.Register([&scopes, &trace] {
                trace.push_back(2);
                scopes.Register([&trace] {
                    trace.push_back(3);
                    return utils::FastScopeGuard([&trace]() noexcept { trace.push_back(4); });
                });
                return utils::FastScopeGuard([&trace]() noexcept { trace.push_back(5); });
            });
            trace.push_back(6);
        });
    });

    scopes.AfterConstruction();
    scopes.BeforeDestruction();
    EXPECT_THAT(trace, ::testing::ElementsAre(1, 2, 3, 4, 5, 6));
}

TEST(ResourceScopeStorageUnit, RegisterDuringDestructionOpensAndCloses) {
#ifndef NDEBUG
    GTEST_SKIP() << "UASSERT aborts in debug builds";
#endif

    utils::ResourceScopeStorage scopes;
    std::vector<int> trace;

    scopes.AfterConstruction();
    scopes.BeforeDestruction();
    scopes.Register([&trace] {
        trace.push_back(1);
        return utils::FastScopeGuard([&trace]() noexcept { trace.push_back(2); });
    });
    EXPECT_THAT(trace, ::testing::ElementsAre(1, 2));
}

TEST(ResourceScopeStorageDeathTest, RegisterDuringDestructionAborts) {
#ifdef NDEBUG
    GTEST_SKIP() << "UASSERT is a no-op in release builds";
#endif

    utils::ResourceScopeStorage scopes;
    scopes.AfterConstruction();
    scopes.BeforeDestruction();
    UEXPECT_DEATH(scopes.Register([] { return utils::FastScopeGuard([]() noexcept {}); }), "partially destroyed");
}

USERVER_NAMESPACE_END
