#include <userver/storages/redis/parse_reply.hpp>

#include <cmath>

#include <gtest/gtest.h>

#include <userver/storages/redis/exception.hpp>
#include <userver/storages/redis/reply.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

double ParseDouble(std::string value) {
    return storages::redis::Parse(
        storages::redis::ReplyData{std::move(value)},
        "test_request",
        storages::redis::To<double>{}
    );
}

}  // namespace

TEST(ParseReply, DoubleValid) {
    EXPECT_DOUBLE_EQ(ParseDouble("3.14"), 3.14);
    EXPECT_DOUBLE_EQ(ParseDouble("-2.5"), -2.5);
    EXPECT_DOUBLE_EQ(ParseDouble("1e3"), 1000.0);
}

TEST(ParseReply, DoubleInfinity) {
    // ZSCORE and friends report infinite scores as "inf"/"-inf".
    EXPECT_TRUE(std::isinf(ParseDouble("inf")));
    EXPECT_TRUE(std::isinf(ParseDouble("-inf")));
}

TEST(ParseReply, DoubleTrailingJunk) {
    // A compromised, misbehaving, or MITM'd server can return a bulk string with
    // a valid numeric prefix followed by junk. std::stod silently accepted the
    // prefix and dropped the rest; the strict parser rejects the whole reply.
    EXPECT_THROW(ParseDouble("3.14garbage"), storages::redis::ParseReplyException);
    EXPECT_THROW(ParseDouble("nonsense"), storages::redis::ParseReplyException);
    EXPECT_THROW(ParseDouble(""), storages::redis::ParseReplyException);
}

USERVER_NAMESPACE_END
