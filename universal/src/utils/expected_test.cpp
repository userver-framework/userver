#include <cstdint>
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

TEST(Expected, DereferenceOperator) {
    ExpectedInt ei{5};

    EXPECT_EQ(*ei, 5);

    *ei += 10;
    EXPECT_EQ(*ei, 15);

    const ExpectedInt& cei = ei;
    EXPECT_EQ(*cei, 15);

    EXPECT_EQ(*std::move(ei), 15);
}

TEST(Expected, ArrowOperator) {
    utils::expected<std::string, std::string> es{std::string{"hello"}};

    EXPECT_EQ(es->size(), 5u);

    const utils::expected<std::string, std::string>& ces = es;
    EXPECT_EQ(ces->size(), 5u);

    es->push_back('!');
    EXPECT_EQ(es.value(), "hello!");
}

TEST(Expected, EqualityWithAnotherExpected) {
    const ExpectedInt value5{5};
    const ExpectedInt another_value5{5};
    const ExpectedInt value10{10};
    const ExpectedInt error_a{utils::unexpected{std::string("error a")}};
    const ExpectedInt another_error_a{utils::unexpected{std::string("error a")}};
    const ExpectedInt error_b{utils::unexpected{std::string("error b")}};

    EXPECT_EQ(value5, another_value5);
    EXPECT_NE(value5, value10);
    EXPECT_NE(value5, error_a);
    EXPECT_EQ(error_a, another_error_a);
    EXPECT_NE(error_a, error_b);

    // Reversed argument order is synthesized by the compiler (C++20 rewritten operators).
    EXPECT_EQ(another_value5, value5);
}

TEST(Expected, VoidEqualityWithAnotherExpected) {
    const ExpectedVoid void_value1;
    const ExpectedVoid void_value2;
    const ExpectedVoid void_error_a{utils::unexpected{std::string("error a")}};
    const ExpectedVoid void_another_error_a{utils::unexpected{std::string("error a")}};
    const ExpectedVoid void_error_b{utils::unexpected{std::string("error b")}};

    EXPECT_EQ(void_value1, void_value2);
    EXPECT_NE(void_value1, void_error_a);
    EXPECT_EQ(void_error_a, void_another_error_a);
    EXPECT_NE(void_error_a, void_error_b);
}

TEST(Expected, EqualityWithValue) {
    const ExpectedInt value5{5};
    const ExpectedInt error_a{utils::unexpected{std::string("error a")}};

    EXPECT_EQ(value5, 5);
    EXPECT_NE(value5, 10);
    EXPECT_NE(error_a, 5);

    // Reversed argument order is synthesized by the compiler (C++20 rewritten operators).
    EXPECT_EQ(5, value5);
    EXPECT_NE(10, value5);
}

TEST(Expected, EqualityWithValueOfDifferentSignedness) {
    // Regression test: comparing against a value of different signedness must not trigger
    // a -Wsign-compare warning (treated as an error in this project) and must still compare correctly.
    using ExpectedU64 = utils::expected<std::uint64_t, std::string>;
    const ExpectedU64 value5{std::uint64_t{5}};
    const ExpectedU64 error_a{utils::unexpected{std::string("error a")}};

    EXPECT_EQ(value5, 5);
    EXPECT_NE(value5, 6);
    EXPECT_NE(error_a, 5);
    EXPECT_EQ(5, value5);
}

TEST(Expected, EqualityWithUnexpected) {
    const ExpectedInt value5{5};
    const ExpectedInt error_a{utils::unexpected{std::string("error a")}};
    const ExpectedInt another_error_a{utils::unexpected{std::string("error a")}};

    EXPECT_NE(value5, utils::unexpected(std::string("error a")));
    EXPECT_EQ(error_a, utils::unexpected(std::string("error a")));
    EXPECT_EQ(error_a, another_error_a);
    EXPECT_NE(error_a, utils::unexpected(std::string("error b")));

    // Reversed argument order is synthesized by the compiler (C++20 rewritten operators).
    EXPECT_EQ(utils::unexpected(std::string("error a")), error_a);
    EXPECT_NE(utils::unexpected(std::string("error a")), value5);
}

TEST(Expected, VoidEqualityWithUnexpected) {
    const ExpectedVoid void_value;
    const ExpectedVoid void_error_a{utils::unexpected{std::string("error a")}};

    EXPECT_NE(void_value, utils::unexpected(std::string("error a")));
    EXPECT_EQ(void_error_a, utils::unexpected(std::string("error a")));
    EXPECT_NE(void_error_a, utils::unexpected(std::string("error b")));
    EXPECT_EQ(utils::unexpected(std::string("error a")), void_error_a);
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
