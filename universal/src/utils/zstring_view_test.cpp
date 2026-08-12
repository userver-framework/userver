#include <userver/utils/zstring_view.hpp>

#include <concepts>
#include <type_traits>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

static_assert(!std::is_constructible_v<utils::zstring_view, std::string_view>);
static_assert(std::is_convertible_v<utils::zstring_view, std::string_view>);
static_assert(std::is_trivially_destructible_v<utils::zstring_view>);
static_assert(std::is_trivially_copyable_v<utils::zstring_view>);
static_assert(std::is_trivially_copy_assignable_v<utils::zstring_view>);
static_assert(!std::is_assignable_v<utils::zstring_view, std::string_view>);

static_assert(std::is_assignable_v<std::string_view, utils::zstring_view>);

template <typename T>
concept SuffixRemovable = requires(T t) { t.remove_suffix(10); };

static_assert(std::swappable<utils::zstring_view>);
static_assert(!std::swappable_with<utils::zstring_view, std::string_view>);
static_assert(!SuffixRemovable<utils::zstring_view>);

TEST(ZstringView, UnsafeMake) {
    static constexpr utils::zstring_view kShortString = "short";
    static constexpr utils::zstring_view kLongString = "some long long long long long long long long long string";

    EXPECT_EQ(kShortString, "short");
    EXPECT_EQ(kShortString, std::string{"short"});
    EXPECT_EQ(kShortString.c_str(), std::string{"short"});
    EXPECT_EQ(kLongString, std::string{kLongString});
    EXPECT_EQ(kLongString.c_str(), std::string{kLongString});

    static_assert(kShortString == "short");
    static_assert(kShortString != kLongString);

    const char* data = kShortString.data();
    auto size = kShortString.size();
    EXPECT_EQ(kShortString, utils::zstring_view::UnsafeMake(data, size));

    data = kLongString.data();
    size = kLongString.size();
    EXPECT_EQ(kLongString, utils::zstring_view::UnsafeMake(data, size));
    EXPECT_EQ(kShortString, "short");

    static_assert(kLongString == utils::zstring_view::UnsafeMake(kLongString.c_str(), kLongString.size()));
}

TEST(ZstringView, Swap) {
    constexpr utils::zstring_view kShortString = "short";
    constexpr utils::zstring_view kLongString = "some long long long long long long long long long string";

    auto v1 = kShortString;
    auto v2 = kLongString;
    v1.swap(v2);

    EXPECT_EQ(v1, kLongString);
    EXPECT_EQ(v2, kShortString);
}

// For comparison-operator tests (shared typed suite for StringLiteral and zstring_view), see string_literal_test.cpp.

USERVER_NAMESPACE_END
