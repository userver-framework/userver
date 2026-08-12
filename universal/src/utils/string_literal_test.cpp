#include <userver/utils/string_literal.hpp>

#include <concepts>
#include <type_traits>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

static_assert(!std::is_constructible_v<utils::StringLiteral, std::string_view>);
static_assert(std::is_convertible_v<utils::StringLiteral, std::string_view>);
static_assert(std::is_convertible_v<utils::StringLiteral, utils::zstring_view>);
static_assert(std::is_trivially_destructible_v<utils::StringLiteral>);
static_assert(std::is_trivially_copyable_v<utils::StringLiteral>);
static_assert(std::is_trivially_copy_assignable_v<utils::StringLiteral>);
static_assert(!std::is_assignable_v<utils::StringLiteral, std::string_view>);
static_assert(!std::is_assignable_v<utils::StringLiteral, utils::zstring_view>);

static_assert(std::is_assignable_v<std::string_view, utils::StringLiteral>);
static_assert(std::is_assignable_v<utils::zstring_view, utils::StringLiteral>);

template <typename T>
concept SuffixRemovable = requires(T t) { t.remove_suffix(10); };

static_assert(std::swappable<utils::StringLiteral>);
static_assert(!std::swappable_with<utils::StringLiteral, utils::zstring_view>);
static_assert(!std::swappable_with<utils::StringLiteral, std::string_view>);
static_assert(!SuffixRemovable<utils::StringLiteral>);

static constexpr utils::StringLiteral kLongString = "some long long long long long long long long long string";

TEST(StringLiteral, UnsafeMake) {
    static constexpr utils::StringLiteral kShortString = "short";

    EXPECT_EQ(kShortString, "short");
    EXPECT_EQ(kShortString, std::string{"short"});
    EXPECT_EQ(kShortString.c_str(), std::string{"short"});
    EXPECT_EQ(kLongString, std::string{kLongString});
    EXPECT_EQ(kLongString.c_str(), std::string{kLongString});

    static_assert(kShortString == "short");
    static_assert(kShortString != kLongString);

    const char* data = kShortString.data();
    auto size = kShortString.size();
    EXPECT_EQ(kShortString, utils::StringLiteral::UnsafeMake(data, size));

    data = kLongString.data();
    size = kLongString.size();
    EXPECT_EQ(kLongString, utils::StringLiteral::UnsafeMake(data, size));
    EXPECT_EQ(kShortString, "short");

    static_assert(kLongString == utils::StringLiteral::UnsafeMake(kLongString.c_str(), kLongString.size()));
}

TEST(StringLiteral, Swap) {
    constexpr utils::StringLiteral kShortString = "short";
    constexpr utils::StringLiteral kLongString = "some long long long long long long long long long string";

    auto v1 = kShortString;
    auto v2 = kLongString;
    v1.swap(v2);

    EXPECT_EQ(v1, kLongString);
    EXPECT_EQ(v2, kShortString);
}

template <typename T, typename U>
// NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
static constexpr bool AreEqualInEveryWay(T&& lhs, U&& rhs) {
    return lhs == rhs && rhs == lhs &&        //
           !(lhs != rhs) && !(rhs != lhs) &&  //
           !(lhs < rhs) && !(rhs < lhs) &&    //
           !(lhs > rhs) && !(rhs > lhs) &&    //
           (lhs <= rhs) && (rhs <= lhs) &&    //
           (lhs >= rhs) && (rhs >= lhs) &&    //
           (lhs <=> rhs) == std::strong_ordering::equal && (rhs <=> lhs) == std::strong_ordering::equal;
}

template <typename StringLike>
class StringLikeComparisonTest : public testing::Test {};

using StringLikeComparisonTypes = ::testing::Types<utils::StringLiteral, utils::zstring_view>;

TYPED_TEST_SUITE(StringLikeComparisonTest, StringLikeComparisonTypes);

TYPED_TEST(StringLikeComparisonTest, Comparison) {
    static constexpr TypeParam kCmp = "ab";

    static_assert(AreEqualInEveryWay(kCmp, kCmp));
    static_assert(AreEqualInEveryWay(kCmp, "ab"));
    static_assert(AreEqualInEveryWay(kCmp, static_cast<const char(&)[3]>("ab")));
    static_assert(AreEqualInEveryWay(kCmp, static_cast<const char*>("ab")));
    static_assert(AreEqualInEveryWay(kCmp, std::string_view{"ab"}));
    static_assert(AreEqualInEveryWay(kCmp, utils::zstring_view{"ab"}));
    static_assert(AreEqualInEveryWay(kCmp, utils::StringLiteral{"ab"}));
}

USERVER_NAMESPACE_END
