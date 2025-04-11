#include <userver/utils/null_terminated_view.hpp>

#include <type_traits>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

static_assert(!std::is_constructible_v<utils::NullTerminatedView, std::string_view>);
static_assert(std::is_convertible_v<utils::NullTerminatedView, std::string_view>);
static_assert(std::is_trivially_destructible_v<utils::NullTerminatedView>);
static_assert(std::is_trivially_copyable_v<utils::NullTerminatedView>);
static_assert(std::is_trivially_copy_assignable_v<utils::NullTerminatedView>);
static_assert(!std::is_assignable_v<utils::NullTerminatedView, std::string_view>);

static_assert(std::is_assignable_v<std::string_view, utils::NullTerminatedView>);

TEST(NullTerminatedView, UnsafeMake) {
    static constexpr utils::NullTerminatedView kShortString = "short";
    static constexpr utils::NullTerminatedView kLongString = "some long long long long long long long long long string";

    EXPECT_EQ(kShortString, "short");
    EXPECT_EQ(kShortString, std::string{"short"});
    EXPECT_EQ(kShortString.c_str(), std::string{"short"});
    EXPECT_EQ(kLongString, std::string{kLongString});
    EXPECT_EQ(kLongString.c_str(), std::string{kLongString});

    static_assert(kShortString == "short");
    static_assert(kShortString != kLongString);

    const char* data = kShortString.data();
    auto size = kShortString.size();
    EXPECT_EQ(kShortString, utils::NullTerminatedView::UnsafeMake(data, size));

    data = kLongString.data();
    size = kLongString.size();
    EXPECT_EQ(kLongString, utils::NullTerminatedView::UnsafeMake(data, size));
    EXPECT_EQ(kShortString, "short");

    static_assert(kLongString == utils::NullTerminatedView::UnsafeMake(kLongString.c_str(), kLongString.size()));
}

USERVER_NAMESPACE_END
