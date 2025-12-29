#include <string>

#include <gtest/gtest.h>

#include <userver/utils/expected.hpp>

USERVER_NAMESPACE_BEGIN

using ExpectedInt = utils::expected<int, std::string>;
using ExpectedVoid = utils::expected<void, std::string>;

TEST(Expected, DefaultCtorCreatesValue) {
    EXPECT_TRUE(ExpectedInt{}.has_value());
    EXPECT_TRUE(ExpectedInt{});
    EXPECT_EQ(ExpectedInt{}.value(), 0);
    EXPECT_TRUE(ExpectedVoid{}.has_value());
    EXPECT_TRUE(ExpectedVoid{});
}

TEST(Expected, ValueCtor) {
    ExpectedInt ei{5};

    EXPECT_TRUE(ei.has_value());
    EXPECT_TRUE(ei);
    ASSERT_NO_THROW(ei.value());
    EXPECT_EQ(ei.value(), 5);

    ei.value() += 10;

    EXPECT_TRUE(ei.has_value());
    EXPECT_TRUE(ei);
    EXPECT_EQ(std::move(ei).value(), 15);
}

TEST(Expected, ErrorCtor) {
    auto error = utils::unexpected{std::string("string error")};

    ExpectedInt ei{error};
    ExpectedVoid ev{std::move(error)};

    EXPECT_FALSE(ei.has_value());
    EXPECT_FALSE(ei);
    EXPECT_FALSE(ev.has_value());
    EXPECT_FALSE(ev);
    EXPECT_EQ(const_cast<const ExpectedInt&>(ei).error(), "string error");
    EXPECT_EQ(const_cast<const ExpectedVoid&>(ev).error(), "string error");

    ei.error() = "another error";
    ev.error() = "one more error";

    EXPECT_EQ(ei.error(), "another error");
    EXPECT_EQ(ev.error(), "one more error");

    ei = ExpectedInt{utils::unexpected<const char*>("converted error")};
    ev = ExpectedVoid{utils::unexpected<const char*>("converted error")};

    EXPECT_FALSE(ei.has_value());
    EXPECT_FALSE(ei);
    EXPECT_FALSE(ev.has_value());
    EXPECT_FALSE(ev);
    EXPECT_EQ(ei.error(), "converted error");
    EXPECT_EQ(ev.error(), "converted error");
}

TEST(Expected, ValueThrowsIfExpectedContainsError) {
    auto error = utils::unexpected{std::string("string error")};

    ExpectedInt ei{error};
    ExpectedVoid ev{std::move(error)};

    EXPECT_THROW(const_cast<const ExpectedInt&>(ei).value(), utils::bad_expected_access);
    EXPECT_THROW(ei.value(), utils::bad_expected_access);
    EXPECT_THROW(std::move(ei).value(), utils::bad_expected_access);
    EXPECT_THROW(ev.value(), utils::bad_expected_access);
}

TEST(Expected, ErrorThrowsIfExpectedContainsValue) {
    ExpectedInt ei{10};
    ExpectedVoid ev;

    EXPECT_THROW(const_cast<const ExpectedInt&>(ei).error(), utils::bad_expected_access);
    EXPECT_THROW(ei.error(), utils::bad_expected_access);
    EXPECT_THROW(const_cast<const ExpectedVoid&>(ev).error(), utils::bad_expected_access);
    EXPECT_THROW(ev.error(), utils::bad_expected_access);
}

USERVER_NAMESPACE_END
