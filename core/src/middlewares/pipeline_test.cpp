#include <userver/utest/utest.hpp>

#include "impl/middlewares_graph.hpp"

#include <userver/formats/yaml/serialize.hpp>
#include <userver/middlewares/groups.hpp>
#include <userver/middlewares/pipeline.hpp>
#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using Builder = middlewares::MiddlewareDependencyBuilder;
using OrderedList = middlewares::impl::MiddlewareOrderedList;

struct Log final {
    static constexpr std::string_view kName = "grpc-server-logging";
};
struct Baggage final {
    static constexpr std::string_view kName = "grpc-server-baggage";
};
struct Deadline final {
    static constexpr std::string_view kName = "grpc-server-deadline-propagation";
};
struct Congestion final {
    static constexpr std::string_view kName = "grpc-server-congestion-control";
};
struct HeadersPropagator final {
    static constexpr std::string_view kName = "grpc-server-headers-propagator";
};

constexpr auto kStrongConnect = middlewares::DependencyType::kStrong;
constexpr auto kWeakConnect = middlewares::DependencyType::kWeak;

const auto kEmptyConfig = middlewares::impl::MiddlewareRunnerConfig{
    {},
    /* disable_user_group=*/false,
    /* disable_all=*/false,
};

template <typename Middleware>
middlewares::impl::MiddlewareEnabled Mid(bool enabled = true) {
    return {std::string{Middleware::kName}, enabled};
}

template <typename Group, typename Before, typename After>
std::pair<std::string, middlewares::impl::MiddlewareDependency> Dependency(
    middlewares::DependencyType con_type = middlewares::DependencyType::kWeak
) {
    return {
        std::string{Before::kName},
        Builder().InGroup<Group>().template After<After>(con_type).ExtractDependency(Before::kName)};
}

middlewares::impl::Dependencies kDefaultDependencies{
    {std::string{Log::kName}, Builder().InGroup<middlewares::groups::Logging>().ExtractDependency(Log::kName)},
    {std::string{Congestion::kName},
     Builder().InGroup<middlewares::groups::Core>().ExtractDependency(Congestion::kName)},
    {std::string{Deadline::kName},
     Builder().InGroup<middlewares::groups::Core>().After<Congestion>(kWeakConnect).ExtractDependency(Deadline::kName)},
    {std::string{Baggage::kName}, Builder().InGroup<middlewares::groups::User>().ExtractDependency(Baggage::kName)},
    {std::string{HeadersPropagator::kName},
     Builder().InGroup<middlewares::groups::User>().ExtractDependency(HeadersPropagator::kName)},
};

yaml_config::YamlConfig TrueConf() {
    return yaml_config::YamlConfig(formats::yaml::FromString("enabled: true"), formats::yaml::Value{});
}
yaml_config::YamlConfig FalseConf() {
    return yaml_config::YamlConfig(formats::yaml::FromString("enabled: false"), formats::yaml::Value{});
}

struct A1 final {
    static constexpr std::string_view kName = "a1";
};

struct A2 final {
    static constexpr std::string_view kName = "a2";
};

struct C1 final {
    static constexpr std::string_view kName = "c1";
};

struct C2 final {
    static constexpr std::string_view kName = "c2";
};

struct C3 final {
    static constexpr std::string_view kName = "c3";
};

struct U1 final {
    static constexpr std::string_view kName = "u1";
};

struct U2 final {
    static constexpr std::string_view kName = "u2";
};

}  // namespace

TEST(MiddlewarePipeline, Empty) {
    auto dependencies = kDefaultDependencies;
    dependencies.clear();

    const middlewares::impl::MiddlewarePipeline pipeline{std::move(dependencies)};
    const auto& list = pipeline.GetOrderedList();

    ASSERT_TRUE(list.empty());
}

TEST(MiddlewarePipeline, SimpleList) {
    auto dependencies = kDefaultDependencies;

    const middlewares::impl::MiddlewarePipeline pipeline{std::move(dependencies)};
    const auto& list = pipeline.GetOrderedList();

    const OrderedList expected{
        Mid<Log>(),
        Mid<Congestion>(),
        Mid<Deadline>(),
        Mid<Baggage>(),
        Mid<HeadersPropagator>(),
    };

    ASSERT_EQ(expected, list);
}

TEST(MiddlewarePipeline, DisableWeakConnection) {
    auto dependencies = kDefaultDependencies;
    const bool disabled = false;
    dependencies["grpc-server-logging"].enabled = disabled;

    const middlewares::impl::MiddlewarePipeline pipeline{std::move(dependencies)};
    const auto& list = pipeline.GetOrderedList();

    const OrderedList expected{
        Mid<Log>(disabled),
        Mid<Congestion>(),
        Mid<Deadline>(),
        Mid<Baggage>(),
        Mid<HeadersPropagator>(),
    };
    ASSERT_EQ(expected, list);
}

TEST(MiddlewarePipelineDeathTest, DisableStrongConnection) {
    auto dependencies = kDefaultDependencies;

    // disable 'grpc-server-congestion-control'
    dependencies["grpc-server-congestion-control"].enabled = false;
    // Set a Strong connect from 'grpc-server-congestion-control' to 'grpc-server-congestion-control'
    dependencies.erase(std::string{Deadline::kName});
    dependencies.insert(Dependency<middlewares::groups::Core, Deadline, Congestion>(kStrongConnect));

    EXPECT_UINVARIANT_FAILURE(middlewares::impl::BuildPipeline(std::move(dependencies)));
}

TEST(MiddlewarePipelineDeathTest, DependencyToOtherGroup) {
    auto dependencies = kDefaultDependencies;
    // Dependency from User to Core is not allowed
    dependencies.insert(Dependency<middlewares::groups::User, U1, Log>(kWeakConnect));

    EXPECT_UINVARIANT_FAILURE(middlewares::impl::BuildPipeline(std::move(dependencies)));
}

TEST(MiddlewarePipeline, LexicographicOrder) {
    auto dependencies = kDefaultDependencies;
    dependencies.insert(Dependency<middlewares::groups::Logging, C1, Log>());
    dependencies.insert(Dependency<middlewares::groups::Logging, C2, Log>());
    dependencies.insert(Dependency<middlewares::groups::Logging, C3, Log>());

    const middlewares::impl::MiddlewarePipeline pipeline{std::move(dependencies)};
    const auto& list = pipeline.GetOrderedList();

    const OrderedList expected{
        Mid<Log>(),
        Mid<C1>(),
        Mid<C2>(),
        Mid<C3>(),
        Mid<Congestion>(),
        Mid<Deadline>(),
        Mid<Baggage>(),
        Mid<HeadersPropagator>(),
    };
    ASSERT_EQ(expected, list);
}

TEST(MiddlewarePipeline, MultiDependency) {
    auto dependencies = kDefaultDependencies;
    dependencies.insert(
        {std::string{A2::kName},
         Builder().InGroup<middlewares::groups::Core>().After<Congestion>().Before<Deadline>().ExtractDependency(
             A2::kName
         )}
    );

    const middlewares::impl::MiddlewarePipeline pipeline{std::move(dependencies)};
    const auto& list = pipeline.GetOrderedList();

    const OrderedList expected{
        Mid<Log>(),
        Mid<Congestion>(),
        Mid<A2>(),
        Mid<Deadline>(),
        Mid<Baggage>(),
        Mid<HeadersPropagator>(),
    };
    ASSERT_EQ(expected, list);
}

TEST(MiddlewarePipeline, BetweenGroups) {
    auto dependencies = kDefaultDependencies;
    dependencies.insert(
        {std::string{A1::kName},
         Builder().After<middlewares::groups::Logging>().Before<middlewares::groups::Auth>().ExtractGroupDependency(
             A1::kName
         )}
    );

    const middlewares::impl::MiddlewarePipeline pipeline{std::move(dependencies)};
    const auto& list = pipeline.GetOrderedList();

    const OrderedList expected{
        Mid<Log>(),
        Mid<A1>(),
        Mid<Congestion>(),
        Mid<Deadline>(),
        Mid<Baggage>(),
        Mid<HeadersPropagator>(),
    };
    ASSERT_EQ(expected, list);
}

TEST(MiddlewarePipeline, DisablePerService) {
    auto dependencies = kDefaultDependencies;

    dependencies.emplace(
        std::string{U1::kName}, Builder().InGroup<middlewares::groups::User>().ExtractDependency(U1::kName)
    );

    const middlewares::impl::MiddlewarePipeline pipeline{std::move(dependencies)};
    const auto list = pipeline.GetPerServiceMiddlewares(middlewares::impl::MiddlewareRunnerConfig{
        {{
            {std::string{Deadline::kName}, FalseConf()},
            {std::string{U1::kName}, FalseConf()},
        }},
        /* disable_user_group=*/false,
        /* disable_all=*/false,
    });

    const std::vector<std::string> expected{
        std::string{Log::kName},
        std::string{Congestion::kName},
        // Deadline id disabled
        std::string{Baggage::kName},
        std::string{HeadersPropagator::kName},
        // U1 is disabled
    };
    ASSERT_EQ(expected, list);
}

TEST(MiddlewarePipeline, DisableUserGroup) {
    auto dependencies = kDefaultDependencies;

    const middlewares::impl::MiddlewarePipeline pipeline{std::move(dependencies)};
    const auto list = pipeline.GetPerServiceMiddlewares(middlewares::impl::MiddlewareRunnerConfig{
        {{
            {std::string{Baggage::kName}, TrueConf()},
        }},
        /* disable_user_group=*/true,
        /* disable_all=*/false,
    });

    const std::vector<std::string> expected{
        std::string{Log::kName},
        std::string{Congestion::kName},
        std::string{Deadline::kName},
        std::string{Baggage::kName},  // force enable
        // Baggage and HeadersPropagator are disabled
    };
    ASSERT_EQ(expected, list);
}

TEST(MiddlewarePipeline, DisableAllPipelineMiddlewares) {
    auto dependencies = kDefaultDependencies;

    dependencies.emplace(
        std::string{A1::kName}, Builder().InGroup<middlewares::groups::Auth>().After<A2>().ExtractDependency(A1::kName)
    );
    dependencies.emplace(
        std::string{A2::kName}, Builder().InGroup<middlewares::groups::Auth>().ExtractDependency(A2::kName)
    );
    const middlewares::impl::MiddlewarePipeline pipeline{std::move(dependencies)};
    const auto list = pipeline.GetPerServiceMiddlewares(middlewares::impl::MiddlewareRunnerConfig{
        {{
            {std::string{A1::kName}, TrueConf()},
            {std::string{A2::kName}, TrueConf()},
            {std::string{Deadline::kName}, TrueConf()},
        }},
        /* disable_user_group=*/false,
        /* disable_all=*/true,
    });

    // Disable the global pipeline, but local force enabled, so there are middlewares from MiddlewareServiceConfig
    const std::vector<std::string> expected{
        std::string{A2::kName},
        std::string{A1::kName},
        std::string{Deadline::kName},
    };
    ASSERT_EQ(expected, list);
}

TEST(MiddlewarePipeline, DisableAll) {
    auto dependencies = kDefaultDependencies;

    const middlewares::impl::MiddlewarePipeline pipeline{std::move(dependencies)};
    const auto list = pipeline.GetPerServiceMiddlewares(middlewares::impl::MiddlewareRunnerConfig{
        {},
        /* disable_user_group=*/false,
        /* disable_all=*/true,
    });
    ASSERT_TRUE(list.empty());
}

TEST(MiddlewarePipeline, GlobalDisableAndPerServiceEnable) {
    auto dependencies = kDefaultDependencies;
    dependencies["grpc-server-logging"].enabled = false;
    dependencies["grpc-server-headers-propagator"].enabled = false;
    dependencies["grpc-server-baggage"].enabled = false;

    const middlewares::impl::MiddlewarePipeline pipeline{std::move(dependencies)};
    const auto list = pipeline.GetPerServiceMiddlewares(middlewares::impl::MiddlewareRunnerConfig{
        {{
            {std::string{Log::kName}, TrueConf()},
            {std::string{Baggage::kName}, TrueConf()},
        }},
        /* disable_user_group=*/false,
        /* disable_all=*/false,
    });

    const std::vector<std::string> expected{
        std::string{Log::kName},  // force enabled
        std::string{Congestion::kName},
        std::string{Deadline::kName},
        std::string{Baggage::kName},  // force enabled
        // HeadersPropagator is disabled
    };
    ASSERT_EQ(expected, list);
}

TEST(MiddlewarePipeline, DurabilityOrder) {
    auto dependencies = kDefaultDependencies;

    dependencies.emplace(
        std::string{U1::kName},
        Builder().InGroup<middlewares::groups::User>().Before<Baggage>().ExtractDependency(U1::kName)
    );
    dependencies.emplace(
        std::string{U2::kName}, Builder().InGroup<middlewares::groups::User>().Before<U1>().ExtractDependency(U2::kName)
    );
    const middlewares::impl::MiddlewarePipeline pipeline{std::move(dependencies)};
    const auto list = pipeline.GetPerServiceMiddlewares(kEmptyConfig);

    const std::vector<std::string> expected{
        std::string{Log::kName},
        std::string{Congestion::kName},
        std::string{Deadline::kName},
        std::string{HeadersPropagator::kName},
        std::string{U2::kName},
        std::string{U1::kName},
        std::string{Baggage::kName},
    };
    ASSERT_EQ(expected, list);

    /////////////////////////////////////////
    auto dependencies2 = kDefaultDependencies;
    dependencies2.emplace(
        std::string{U1::kName},
        Builder().InGroup<middlewares::groups::User>().Before<Baggage>(kWeakConnect).ExtractDependency(U1::kName)
    );
    dependencies2.emplace(
        std::string{U2::kName},
        Builder().InGroup<middlewares::groups::User>().Before<U1>(kWeakConnect).ExtractDependency(U2::kName)
    );
    // Disable 'u1' and now 'u2' is not Before<Baggage> but the order is durability, because `enabled` does not affect
    // connects
    dependencies2["u1"].enabled = false;
    const middlewares::impl::MiddlewarePipeline pipeline2{std::move(dependencies2)};
    const auto list2 = pipeline2.GetPerServiceMiddlewares(kEmptyConfig);

    const std::vector<std::string> expected2{
        std::string{Log::kName},
        std::string{Congestion::kName},
        std::string{Deadline::kName},
        std::string{HeadersPropagator::kName},
        std::string{U2::kName},  // There is a phantom disabled node 'u1', so U2 is still before Baggage
        std::string{Baggage::kName},
    };
    ASSERT_EQ(expected2, list2);
}

USERVER_NAMESPACE_END
