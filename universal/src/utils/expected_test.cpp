#include <string>
#include <utility>

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
    ASSERT_TRUE(ei.has_error());
    ASSERT_TRUE(ev.has_error());
    ASSERT_NO_THROW(ei.error());
    ASSERT_NO_THROW(ev.error());
    EXPECT_EQ(std::as_const(ei).error(), "string error");
    EXPECT_EQ(std::as_const(ev).error(), "string error");

    ei.error() = "another error";
    ev.error() = "one more error";

    EXPECT_EQ(ei.error(), "another error");
    EXPECT_EQ(ev.error(), "one more error");

    auto error2 = utils::unexpected<const char*>("converted error");

    ei = ExpectedInt{error2};
    ev = ExpectedVoid{std::move(error2)};

    EXPECT_FALSE(ei.has_value());
    EXPECT_FALSE(ei);
    EXPECT_FALSE(ev.has_value());
    EXPECT_FALSE(ev);
    ASSERT_TRUE(ei.has_error());
    ASSERT_TRUE(ev.has_error());
    ASSERT_NO_THROW(ei.error());
    ASSERT_NO_THROW(ev.error());
    EXPECT_EQ(ei.error(), "converted error");
    EXPECT_EQ(ev.error(), "converted error");
}

TEST(Expected, ValueThrowsIfExpectedContainsNoValue) {
    auto error = utils::unexpected{std::string("string error")};

    ExpectedInt ei{error};
    ExpectedVoid ev{std::move(error)};

    ASSERT_FALSE(ei.has_value());
    ASSERT_FALSE(ev.has_value());
    EXPECT_THROW(std::as_const(ei).value(), utils::bad_expected_access);
    EXPECT_THROW(ei.value(), utils::bad_expected_access);
    EXPECT_THROW(std::move(ei).value(), utils::bad_expected_access);
    EXPECT_THROW(ev.value(), utils::bad_expected_access);
}

TEST(Expected, ErrorThrowsIfExpectedContainsNoError) {
    ExpectedInt ei{10};
    ExpectedVoid ev;

    ASSERT_FALSE(ei.has_error());
    ASSERT_FALSE(ev.has_error());
    EXPECT_THROW(std::as_const(ei).error(), utils::bad_expected_access);
    EXPECT_THROW(ei.error(), utils::bad_expected_access);
    EXPECT_THROW(std::as_const(ev).error(), utils::bad_expected_access);
    EXPECT_THROW(ev.error(), utils::bad_expected_access);
}

TEST(Expected, ValuelessByException) {
    struct Throw {
        Throw() = default;
        Throw(const Throw&) { throw 0; }
        Throw& operator=(const Throw&) { throw 0; }
    };
    using Expected = utils::expected<Throw, int>;

    Expected e(utils::unexpected(0));
    try {
        e = Expected{};
        FAIL();
    } catch (...) {}

    EXPECT_FALSE(e.has_value());
    EXPECT_THROW(e.value(), utils::bad_expected_access);
    EXPECT_FALSE(e.has_error());
    EXPECT_THROW(e.error(), utils::bad_expected_access);
}

USERVER_NAMESPACE_END
