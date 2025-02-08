#include <userver/utest/utest.hpp>

#include <ugrpc/impl/middlewares_graph.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/ugrpc/middlewares/pipeline.hpp>
#include <userver/ugrpc/server/middlewares/groups.hpp>

#include <userver/ugrpc/server/middlewares/baggage/component.hpp>
#include <userver/ugrpc/server/middlewares/congestion_control/component.hpp>
#include <userver/ugrpc/server/middlewares/deadline_propagation/component.hpp>
#include <userver/ugrpc/server/middlewares/headers_propagator/component.hpp>
#include <userver/ugrpc/server/middlewares/log/component.hpp>

#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using Builder = ugrpc::middlewares::MiddlewareDependencyBuilder;
using OrderedList = ugrpc::middlewares::impl::MiddlewareOrderedList;

using Log = ugrpc::server::middlewares::log::Component;
using Baggage = ugrpc::server::middlewares::baggage::Component;
using Deadline = ugrpc::server::middlewares::deadline_propagation::Component;
using Congestion = ugrpc::server::middlewares::congestion_control::Component;
using HeadersPropagator = ugrpc::server::middlewares::headers_propagator::Component;

constexpr auto kStrongConnect = ugrpc::middlewares::DependencyType::kStrong;
constexpr auto kWeakConnect = ugrpc::middlewares::DependencyType::kWeak;

const auto kEmptyConfig = ugrpc::middlewares::impl::MiddlewareRunnerConfig{
    {},
    /* disable_user_group=*/false,
    /* disable_all=*/false,
};

template <typename Middleware>
ugrpc::middlewares::impl::MiddlewareEnabled Mid(bool enabed = true) {
    return {std::string{Middleware::kName}, enabed};
}

template <typename Group, typename Before, typename After>
std::pair<std::string, ugrpc::middlewares::impl::MiddlewareDependency> Dependency(
    ugrpc::middlewares::DependencyType con_type = ugrpc::middlewares::DependencyType::kWeak
) {
    return {
        std::string{Before::kName}, Builder().InGroup<Group>().template After<After>(con_type).Extract(Before::kName)};
}

ugrpc::middlewares::impl::Dependencies kDefaultDependencies{
    {std::string{Log::kName}, Builder().InGroup<ugrpc::server::groups::Logging>().Extract(Log::kName)},
    {std::string{Congestion::kName}, Builder().InGroup<ugrpc::server::groups::Core>().Extract(Congestion::kName)},
    {std::string{Deadline::kName},
     Builder().InGroup<ugrpc::server::groups::Core>().After<Congestion>(kWeakConnect).Extract(Deadline::kName)},
    {std::string{Baggage::kName}, Builder().InGroup<ugrpc::server::groups::User>().Extract(Baggage::kName)},
    {std::string{HeadersPropagator::kName},
     Builder().InGroup<ugrpc::server::groups::User>().Extract(HeadersPropagator::kName)},
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

    const ugrpc::middlewares::impl::MiddlewarePipeline pipeline{std::move(dependencies)};
    const auto& list = pipeline.GetOrderedList();

    ASSERT_TRUE(list.empty());
}

TEST(MiddlewarePipeline, SimpleList) {
    auto dependencies = kDefaultDependencies;

    const ugrpc::middlewares::impl::MiddlewarePipeline pipeline{std::move(dependencies)};
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

    const ugrpc::middlewares::impl::MiddlewarePipeline pipeline{std::move(dependencies)};
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

TEST(MiddlewarePipeline, DisableStrongConnection) {
    auto dependencies = kDefaultDependencies;

    // disable 'grpc-server-congestion-control'
    dependencies["grpc-server-congestion-control"].enabled = false;
    // Set a Strong connect from 'grpc-server-congestion-control' to 'grpc-server-congestion-control'
    dependencies.erase(std::string{Deadline::kName});
    dependencies.insert(Dependency<ugrpc::server::groups::Core, Deadline, Congestion>(kStrongConnect));

    EXPECT_UINVARIANT_FAILURE(ugrpc::middlewares::impl::BuildPipeline(std::move(dependencies)));
}

TEST(MiddlewarePipeline, DependencyToOtherGroup) {
    auto dependencies = kDefaultDependencies;
    // Dependency from User to Core is not allowed
    dependencies.insert(Dependency<ugrpc::server::groups::User, U1, Log>(kWeakConnect));

    EXPECT_UINVARIANT_FAILURE(ugrpc::middlewares::impl::BuildPipeline(std::move(dependencies)));
}

TEST(MiddlewarePipeline, LexicographicOrder) {
    auto dependencies = kDefaultDependencies;
    dependencies.insert(Dependency<ugrpc::server::groups::Logging, C1, Log>());
    dependencies.insert(Dependency<ugrpc::server::groups::Logging, C2, Log>());
    dependencies.insert(Dependency<ugrpc::server::groups::Logging, C3, Log>());

    const ugrpc::middlewares::impl::MiddlewarePipeline pipeline{std::move(dependencies)};
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
         Builder().InGroup<ugrpc::server::groups::Core>().After<Congestion>().Before<Deadline>().Extract(A2::kName)}
    );

    const ugrpc::middlewares::impl::MiddlewarePipeline pipeline{std::move(dependencies)};
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
         Builder().After<ugrpc::server::groups::Logging>().Before<ugrpc::server::groups::Auth>().Extract(A1::kName)}
    );

    const ugrpc::middlewares::impl::MiddlewarePipeline pipeline{std::move(dependencies)};
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

    dependencies.emplace(std::string{U1::kName}, Builder().InGroup<ugrpc::server::groups::User>().Extract(U1::kName));

    const ugrpc::middlewares::impl::MiddlewarePipeline pipeline{std::move(dependencies)};
    const auto list = pipeline.GetPerServiceMiddlewares(ugrpc::middlewares::impl::MiddlewareRunnerConfig{
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

    const ugrpc::middlewares::impl::MiddlewarePipeline pipeline{std::move(dependencies)};
    const auto list = pipeline.GetPerServiceMiddlewares(ugrpc::middlewares::impl::MiddlewareRunnerConfig{
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
        std::string{A1::kName}, Builder().InGroup<ugrpc::server::groups::Auth>().After<A2>().Extract(A1::kName)
    );
    dependencies.emplace(std::string{A2::kName}, Builder().InGroup<ugrpc::server::groups::Auth>().Extract(A2::kName));
    const ugrpc::middlewares::impl::MiddlewarePipeline pipeline{std::move(dependencies)};
    const auto list = pipeline.GetPerServiceMiddlewares(ugrpc::middlewares::impl::MiddlewareRunnerConfig{
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

    const ugrpc::middlewares::impl::MiddlewarePipeline pipeline{std::move(dependencies)};
    const auto list = pipeline.GetPerServiceMiddlewares(ugrpc::middlewares::impl::MiddlewareRunnerConfig{
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

    const ugrpc::middlewares::impl::MiddlewarePipeline pipeline{std::move(dependencies)};
    const auto list = pipeline.GetPerServiceMiddlewares(ugrpc::middlewares::impl::MiddlewareRunnerConfig{
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
        std::string{U1::kName}, Builder().InGroup<ugrpc::server::groups::User>().Before<Baggage>().Extract(U1::kName)
    );
    dependencies.emplace(
        std::string{U2::kName}, Builder().InGroup<ugrpc::server::groups::User>().Before<U1>().Extract(U2::kName)
    );
    const ugrpc::middlewares::impl::MiddlewarePipeline pipeline{std::move(dependencies)};
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
        Builder().InGroup<ugrpc::server::groups::User>().Before<Baggage>(kWeakConnect).Extract(U1::kName)
    );
    dependencies2.emplace(
        std::string{U2::kName},
        Builder().InGroup<ugrpc::server::groups::User>().Before<U1>(kWeakConnect).Extract(U2::kName)
    );
    // Disable 'u1' and now 'u2' is not Before<Baggage> but the order is durability, because `enabled` does not affect
    // connects
    dependencies2["u1"].enabled = false;
    const ugrpc::middlewares::impl::MiddlewarePipeline pipeline2{std::move(dependencies2)};
    const auto list2 = pipeline2.GetPerServiceMiddlewares(kEmptyConfig);

    std::vector<std::string> expected2{
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
