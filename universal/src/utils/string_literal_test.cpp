#include <userver/utils/string_literal.hpp>

#include <type_traits>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

static_assert(!std::is_constructible_v<utils::StringLiteral, std::string_view>);
static_assert(std::is_convertible_v<utils::StringLiteral, std::string_view>);
static_assert(std::is_convertible_v<utils::StringLiteral, utils::NullTerminatedView>);
static_assert(std::is_trivially_destructible_v<utils::StringLiteral>);
static_assert(std::is_trivially_copyable_v<utils::StringLiteral>);
static_assert(std::is_trivially_copy_assignable_v<utils::StringLiteral>);
static_assert(!std::is_assignable_v<utils::StringLiteral, std::string_view>);
static_assert(!std::is_assignable_v<utils::StringLiteral, utils::NullTerminatedView>);

static_assert(std::is_assignable_v<std::string_view, utils::StringLiteral>);
static_assert(std::is_assignable_v<utils::NullTerminatedView, utils::StringLiteral>);

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

USERVER_NAMESPACE_END
