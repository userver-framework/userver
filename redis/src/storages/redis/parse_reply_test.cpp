#include <userver/storages/redis/parse_reply.hpp>

#include <cmath>

#include <gtest/gtest.h>

#include <userver/storages/redis/exception.hpp>
#include <userver/storages/redis/reply.hpp>
#include <userver/storages/redis/reply_types.hpp>

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

// A GEORADIUS/GEOSEARCH reply with WITHDIST/WITHCOORD/WITHHASH is an array of
// per-member arrays whose first element is the member name (a bulk string). A
// malicious, compromised or MITM'd Redis server can return a non-string there.
// The first element used to be read with GetString() without a type check, so
// the parser dereferenced the null returned by std::get_if in a release build.
TEST(ParseReply, GeoPointFirstElementNotString) {
    using storages::redis::ReplyData;

    ReplyData::Array member_info;
    member_info.emplace_back(ReplyData(42));                  // member name, but an int
    member_info.emplace_back(ReplyData(std::string{"1.5"}));  // distance

    ReplyData::Array top;
    top.emplace_back(ReplyData(std::move(member_info)));

    ReplyData reply{std::move(top)};

    EXPECT_THROW(
        storages::redis::ParseReplyDataArray(
            std::move(reply),
            "GEORADIUS",
            storages::redis::To<std::vector<storages::redis::GeoPoint>>{}
        ),
        storages::redis::ParseReplyException
    );
}

TEST(ParseReply, GeoPointFirstElementString) {
    using storages::redis::ReplyData;

    ReplyData::Array member_info;
    member_info.emplace_back(ReplyData(std::string{"Palermo"}));
    member_info.emplace_back(ReplyData(std::string{"190.4424"}));

    ReplyData::Array top;
    top.emplace_back(ReplyData(std::move(member_info)));

    ReplyData reply{std::move(top)};

    auto result = storages::redis::ParseReplyDataArray(
        std::move(reply),
        "GEORADIUS",
        storages::redis::To<std::vector<storages::redis::GeoPoint>>{}
    );
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].member, "Palermo");
}

USERVER_NAMESPACE_END
