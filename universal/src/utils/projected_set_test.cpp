#include <userver/utils/projected_set.hpp>

#include <algorithm>
#include <string>
#include <utility>

#include <gtest/gtest.h>

#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

/// [user]
struct User {
    std::string username;
    std::string first_name;
    int age{};
};

inline constexpr auto kUserKey = &User::username;

using UserUnorderedSet = utils::ProjectedUnorderedSet<User, kUserKey>;
using UserSet = utils::ProjectedSet<User, kUserKey>;
/// [user]

using UserUnorderedSetUsage [[maybe_unused]] = UserUnorderedSet;
using UserSetUsage [[maybe_unused]] = UserSet;

using Pair = std::pair<int, std::string>;
// Decay to function pointer is required in pre-C++20 builds.
constexpr auto kFirst = +[](const Pair& pair) -> auto& { return pair.first; };
constexpr auto kSecond = +[](const Pair& pair) { return pair.second; };

struct UnorderedKind final {
    template <typename Value, auto Projection>
    using type = utils::ProjectedUnorderedSet<Value, Projection>;
};

struct OrderedKind final {
    template <typename Value, auto Projection>
    using type = utils::ProjectedSet<Value, Projection>;
};

template <typename>
class ProjectedSet : public testing::Test {};

using ProjectedSetTypes = ::testing::Types<UnorderedKind, OrderedKind>;

TYPED_TEST_SUITE(ProjectedSet, ProjectedSetTypes);

}  // namespace

TYPED_TEST(ProjectedSet, Sample) {
    using UserProjectedSet = typename TypeParam::template type<User, kUserKey>;

    /// [usage]
    UserProjectedSet set;
    set.insert({"fabulous_tony", "Anthony", 17});
    set.insert({"sassy_nancy", "Anne", 71});

    EXPECT_EQ(utils::ProjectedFind(set, "sassy_nancy")->first_name, "Anne");
    EXPECT_EQ(utils::ProjectedFind(set, "meaning_of_life"), set.end());
    /// [usage]
}

TYPED_TEST(ProjectedSet, LookupOnlyComparesProjection) {
    using Set = typename TypeParam::template type<Pair, kFirst>;
    const std::vector<Pair> original{{10, "x"}};

    auto set = utils::AsContainer<Set>(original);
    EXPECT_FALSE(set.insert({10, "y"}).second);
    EXPECT_EQ(utils::AsContainer<std::vector<Pair>>(set), original);

    // While usually not very appropriate logically, lookup can be performed using
    // the full value. This is an artifact of using std::set instead of a custom
    // container.
    //
    // In this case the projected value is extracted, then looked up as usual.
    const auto iter = utils::ProjectedFind(set, Pair{10, "z"});
    EXPECT_NE(iter, set.end());
    EXPECT_EQ(*iter, Pair(10, "x"));
}

TEST(ProjectedSet, OrderedByProjection) {
    const utils::ProjectedSet<Pair, kSecond> ordered_set{{1, "c"}, {2, "b"}, {3, "a"}};
    const auto as_vector = utils::AsContainer<std::vector<Pair>>(ordered_set);

    const std::vector<Pair> expected{{3, "a"}, {2, "b"}, {1, "c"}};
    EXPECT_EQ(as_vector, expected);
    EXPECT_FALSE(std::is_sorted(as_vector.begin(), as_vector.end()));
}

TYPED_TEST(ProjectedSet, ProjectedInsertOrAssign) {
    using Set = typename TypeParam::template type<Pair, kFirst>;

    Set set;
    EXPECT_EQ(set.size(), 0);

    utils::ProjectedInsertOrAssign(set, Pair{1, "a"});
    EXPECT_EQ(set.size(), 1);
    EXPECT_EQ(utils::ProjectedFind(set, 1)->second, "a");

    utils::ProjectedInsertOrAssign(set, Pair{1, "b"});
    EXPECT_EQ(set.size(), 1);
    EXPECT_EQ(utils::ProjectedFind(set, 1)->second, "b");
}

namespace {

// gcc-9 and gcc-11 get confused when using this function in userver-core. Unit tests are fine, though.
template <typename Value, auto Projection>
decltype(auto) ProjectedUnwrapThen(const utils::impl::MutableWrapper<Value>& value) {
    return std::invoke(Projection, std::as_const(*value));
}

}  // namespace

TYPED_TEST(ProjectedSet, ProjectedFindOrNullptrUnsafe) {
    using Set = typename TypeParam::template type<utils::impl::MutableWrapper<Pair>, ProjectedUnwrapThen<Pair, kFirst>>;
    const std::vector<Pair> original{{10, "x"}};

    auto set = utils::AsContainer<Set>(original);

    Pair* const value = utils::impl::ProjectedFindOrNullptrUnsafe(set, 10);
    ASSERT_TRUE(value);
    // item->first = 20;  // UB!
    value->second = "y";

    const auto iter = utils::ProjectedFind(set, 10);
    ASSERT_TRUE(iter != set.end());
    EXPECT_EQ(**iter, Pair(10, "y"));
}

TYPED_TEST(ProjectedSet, ProjectedUnsafeIteration) {
    using Set = typename TypeParam::template type<utils::impl::MutableWrapper<Pair>, ProjectedUnwrapThen<Pair, kFirst>>;
    const std::vector<Pair> original{{10, "x"}};

    auto set = utils::AsContainer<Set>(original);

    for (auto& item : set) {
        // item->first = 20;  // UB!
        item->second = "y";
    }

    const auto iter = utils::ProjectedFind(set, 10);
    ASSERT_TRUE(iter != set.end());
    EXPECT_EQ(**iter, Pair(10, "y"));
}

USERVER_NAMESPACE_END
