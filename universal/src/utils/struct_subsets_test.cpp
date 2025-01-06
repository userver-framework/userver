#include <userver/utils/struct_subsets.hpp>

#include <gtest/gtest.h>

#include <userver/utils/from_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace sample {

struct SomeClient {};

/// [deps definitions]
struct Dependencies {
    int a;
    int b;
    std::string c;
    SomeClient& d;

    USERVER_ALLOW_CONVERSIONS_TO_SUBSET();
};

USERVER_DEFINE_STRUCT_SUBSET(SmolDependencies, Dependencies, a, c, d);

int Foo(SmolDependencies&& smol_dependencies) {
    return smol_dependencies.a + utils::FromString<int>(smol_dependencies.c);
}
/// [deps definitions]

static_assert(std::is_aggregate_v<Dependencies>);
static_assert(std::is_aggregate_v<SmolDependencies>);

}  // namespace sample

TEST(DefineStructSubset, Sample) {
    using sample::Dependencies, sample::SmolDependencies;

    /// [deps usage]
    sample::SomeClient some_client;

    // Suppose that some common utility function produces Dependencies struct with
    // a lot of fields. However, we don't actually need all of them for Foo.
    Dependencies dependencies{1, 2, "3", some_client};

    // SmolDependencies is implicitly convertible from Dependencies, conversion by
    // copy and by move is supported.
    EXPECT_EQ(Foo(std::move(dependencies)), 4);
    /// [deps usage]
}

struct NonMovable {
    NonMovable() = default;

    NonMovable(NonMovable&&) = delete;
    NonMovable& operator=(NonMovable&&) = delete;
};

namespace copyable {

struct Dependencies {
    int a;
    int b;
    NonMovable& c;

    USERVER_ALLOW_CONVERSIONS_TO_SUBSET();
};

USERVER_DEFINE_STRUCT_SUBSET(DepsCopy, Dependencies, a, b, c);
USERVER_DEFINE_STRUCT_SUBSET(SmolDeps, Dependencies, a, c);
USERVER_DEFINE_STRUCT_SUBSET(TinyDeps, SmolDeps, c);

}  // namespace copyable

TEST(DefineStructSubset, ExactCopy) {
    using copyable::Dependencies, copyable::DepsCopy;

    NonMovable c;
    const Dependencies dependencies{1, 2, c};

    const DepsCopy subset = dependencies;
    EXPECT_EQ(subset.a, dependencies.a);
    EXPECT_EQ(subset.b, dependencies.b);
    EXPECT_EQ(&subset.c, &dependencies.c);

    const DepsCopy from_scratch{4, 5, c};
    EXPECT_EQ(from_scratch.a, 4);
    EXPECT_EQ(from_scratch.b, 5);
    EXPECT_EQ(&from_scratch.c, &c);
}

TEST(DefineStructSubset, SmolDeps) {
    using copyable::Dependencies, copyable::SmolDeps, copyable::TinyDeps;

    NonMovable c;
    const Dependencies dependencies{1, 2, c};

    const SmolDeps subset = dependencies;
    EXPECT_EQ(subset.a, dependencies.a);
    EXPECT_EQ(&subset.c, &dependencies.c);

    const TinyDeps tiny = subset;
    EXPECT_EQ(&tiny.c, &dependencies.c);
}

namespace non_copyable {

struct Dependencies {
    int a;
    std::unique_ptr<int> b;
    NonMovable& c;

    USERVER_ALLOW_CONVERSIONS_TO_SUBSET();
};

USERVER_DEFINE_STRUCT_SUBSET(SmolCopyable, Dependencies, a, c);
USERVER_DEFINE_STRUCT_SUBSET(SmolNonCopyable, Dependencies, b, c);

}  // namespace non_copyable

TEST(DefineStructSubset, NonCopyableToCopyable) {
    using non_copyable::Dependencies, non_copyable::SmolCopyable;

    NonMovable c;
    Dependencies dependencies{1, std::make_unique<int>(2), c};

    const SmolCopyable copyable_subset = dependencies;
    EXPECT_EQ(copyable_subset.a, dependencies.a);
    EXPECT_EQ(&copyable_subset.c, &dependencies.c);

    // Redundant as all moved fields are trivial, but should work.
    const SmolCopyable copyable_subset_move = std::move(dependencies);
    EXPECT_EQ(copyable_subset_move.a, 1);
    // b is not touched, as it's not in SmolCopyable.
    ASSERT_TRUE(dependencies.b);
    EXPECT_EQ(*dependencies.b, 2);
    EXPECT_EQ(&copyable_subset_move.c, &c);
}

TEST(DefineStructSubset, NonCopyableToNonCopyable) {
    using non_copyable::Dependencies, non_copyable::SmolNonCopyable;

    NonMovable c;
    Dependencies dependencies{1, std::make_unique<int>(2), c};

    // const Dependencies& is not convertible to SmolNonCopyable

    const SmolNonCopyable non_copyable_subset = std::move(dependencies);
    ASSERT_TRUE(non_copyable_subset.b);
    EXPECT_EQ(*non_copyable_subset.b, 2);
    EXPECT_FALSE(dependencies.b);
    EXPECT_EQ(&non_copyable_subset.c, &c);
}

namespace non_movable {

struct Dependencies {
    std::unique_ptr<int> a;
    NonMovable b;

    USERVER_ALLOW_CONVERSIONS_TO_SUBSET();
};

USERVER_DEFINE_STRUCT_SUBSET(SmolMovable, Dependencies, a);
USERVER_DEFINE_STRUCT_SUBSET(SmolNonMovable, Dependencies, b);

}  // namespace non_movable

TEST(DefineStructSubset, NonMovableToMovable) {
    using non_movable::Dependencies, non_movable::SmolMovable;

    Dependencies dependencies{std::make_unique<int>(2), NonMovable{}};

    // const Dependencies& is not convertible to SmolMovable

    const SmolMovable movable_subset = std::move(dependencies);
    ASSERT_TRUE(movable_subset.a);
    EXPECT_EQ(*movable_subset.a, 2);
    EXPECT_FALSE(dependencies.a);

    // Dependencies&& is not convertible to SmolNonMovable
}

namespace ref {

/// [ref definitions]
struct Dependencies {
    std::string a;
    int b;
    int& c;
    NonMovable& d;

    USERVER_ALLOW_CONVERSIONS_TO_SUBSET();
};

USERVER_DEFINE_STRUCT_SUBSET(SmolDeps, Dependencies, a, c, d);
USERVER_DEFINE_STRUCT_SUBSET_REF(SmolDepsRef, Dependencies, a, c, d);

// Service-wide utility functions should typically accept Ref dependencies to
// avoid copying members.
int Foo(const SmolDepsRef& dependencies) { return utils::FromString<int>(dependencies.a) + dependencies.c; }

// Imitates launching a BackgroundTaskStorage task.
// Needs non-Ref dependencies, otherwise dangling cache references will happen.
int DoAsync(SmolDeps dependencies) { return utils::FromString<int>(dependencies.a) + dependencies.c; }

// The conversion automatically copies data referenced to by SmolDepsRef members
// if needed.
int Bar(const SmolDepsRef& dependencies) {
    // SmolDepsRef is non-movable, forcing you to either:
    // 1. capture 'dependencies' by reference
    //    (you are responsible for making sure the task does not escape), or
    // 2. copy data members into new non-ref Dependencies to pass inside the task
    return DoAsync(dependencies);
}
/// [ref definitions]

}  // namespace ref

TEST(DefineStructSubsetRef, Sample) {
    using ref::Dependencies, ref::SmolDeps, ref::SmolDepsRef, ref::Foo;

    /// [ref usage]
    int c = 3;
    NonMovable d;
    const Dependencies dependencies{"1", 2, c, d};

    EXPECT_EQ(Foo(dependencies), 4);
    EXPECT_EQ(Bar(dependencies), 4);
    /// [ref usage]
}

}  // namespace

USERVER_NAMESPACE_END
