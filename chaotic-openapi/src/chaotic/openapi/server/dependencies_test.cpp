#include <userver/chaotic/openapi/server/dependencies.hpp>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace dependencies = chaotic::openapi::server::dependencies;

struct EmptyDep {
    bool operator==(const EmptyDep&) const = default;
};

const dependencies::FactoryTag<EmptyDep> kEmptyDep;

struct ForHandlerTag {};

TEST(ChaoticServerDependencies, Smoke) {
    dependencies::Factories fb;
    fb.Register([] { return EmptyDep{}; });

    auto handler = fb.Make<ForHandlerTag>();
    [[maybe_unused]] auto dep = handler[kEmptyDep];
    SUCCEED();
}

// [same type]
// Register the same dependency types in FactoryTag
// with different 'Tag' template parameters as global variables
struct D1 {};
const dependencies::FactoryTag<int, D1> kDep1;
struct D2 {};
const dependencies::FactoryTag<int, D2> kDep2;
// [same type]

// [FactoryTag]
// Register any types in FactoryTag as global variables
const dependencies::FactoryTag<double> kDouble;
const dependencies::FactoryTag<int> kInt;
// [FactoryTag]

struct SameTag {};

TEST(ChaoticServerDependencies, SameType) {
    dependencies::Factories fb;
    fb.Register<D1>([] { return 1; });
    fb.Register<D2>([] { return 2; });

    auto handler = fb.Make<SameTag>();

    // Get the same type, but with different "public" types

    int dep1{handler[kDep1]};
    EXPECT_EQ(dep1, 1);

    int dep2{handler[kDep2]};
    EXPECT_EQ(dep2, 2);
}

TEST(ChaoticServerDependencies, ExplicitValues) {
    dependencies::Factories fb;

    // [ExplicitValues]
    namespace dependencies = chaotic::openapi::server::dependencies;

    // Pass dependencies as-is into ForHandler constructor
    dependencies::ForHandler<struct ExplicitValues> handler{
        1,
        3.3,
    };

    // Fetch dependencies from ForHandler
    EXPECT_EQ(handler[kInt], 1);
    ASSERT_DOUBLE_EQ(handler[kDouble], 3.3);
    // [ExplicitValues]
}

TEST(ChaoticServerDependencies, ExplicitValuesWithTag) {
    dependencies::Factories fb;

    // [WithTag]
    namespace dependencies = chaotic::openapi::server::dependencies;

    // Pass dependencies wrapped with 'FactoryTag' into 'ForHandler' constructor
    dependencies::ForHandler<struct ExplicitValues> handler{
        kDep1.Wrap(1),
        kDep2.Wrap(2),
    };

    // Fetch dependencies from 'ForHandler'
    EXPECT_EQ(handler[kDep1], 1);
    EXPECT_EQ(handler[kDep2], 2);
    // [WithTag]
}

UTEST_DEATH(ChaoticServerDependenciesDeathTest, NotRegisteredInForHandler) {
    dependencies::Factories fb;

    dependencies::ForHandler<struct NotRegisteredInForHandler> handler{1};

    EXPECT_UINVARIANT_FAILURE_MSG(
        handler[kEmptyDep],
        "Trying to access non-registered dependency of type EmptyDep from ForHandler<NotRegisteredInForHandler>."
    );
}

UTEST_DEATH(ChaoticServerDependenciesDeathTest, NotRegisteredInFactory) {
    dependencies::Factories fb;
    // Not registering any builders

    EXPECT_UINVARIANT_FAILURE_MSG(
        (void)(fb.Make<ForHandlerTag>()),
        "Trying to build type EmptyDep from Factories, but the builder is not registered. Forgot to call "
        "Factories::Register<EmptyDep>(...)?"
    );

    if (false) {
        // Never reaches, written to statically register EmptyDep type usage
        // in ForHandler<ForHandlerTag> repo
        auto handler = fb.Make<ForHandlerTag>();
        (void)handler[kEmptyDep];
    }
}

USERVER_NAMESPACE_END
