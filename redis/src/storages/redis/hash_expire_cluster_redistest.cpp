#include <storages/redis/client_cluster_redistest.hpp>

#include <chrono>
#include <string>
#include <vector>

#include <userver/storages/redis/hexpiretime_reply.hpp>
#include <userver/storages/redis/reply_types.hpp>
#include <userver/storages/redis/ttl_reply.hpp>
#include <userver/utils/datetime.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const storages::redis::CommandControl kDefaultCc{
    std::chrono::milliseconds(300),
    std::chrono::milliseconds(300),
    1,
};

/// Use this CommandControl for read commands that follow a write in tests,
/// to avoid replication lag causing flaky failures.
const storages::redis::CommandControl kMasterCC = [] {
    auto cc = kDefaultCc;
    cc.force_request_to_master = true;
    return cc;
}();

std::chrono::system_clock::time_point FutureDeadlineUtc() {
    return utils::datetime::UtcStringtime("2067-06-07T06:07:00Z", utils::datetime::kIsoFormat);
}

std::chrono::system_clock::time_point FutureDeadlineUtcWithMilliseconds() {
    return FutureDeadlineUtc() + std::chrono::milliseconds{123};
}

}  // namespace

UTEST_F(RedisClusterClientTest, HexpireBasicSec) {
    if (!HasHashExpireCommands()) {
        GTEST_SKIP() << SkipMsgHashExpireUnsupported("HEXPIRE");
    }

    auto client = GetClient();
    const std::string key = "{hash_expire}:basic_sec";

    UASSERT_NO_THROW(client->Hset(key, "f1", "v1", kDefaultCc).Get());
    UASSERT_NO_THROW(client->Hset(key, "f2", "v2", kDefaultCc).Get());

    // Set TTL on existing field (seconds-precision)
    {
        auto reply = client->Hexpire(key, std::chrono::seconds(60), {"f1"}, kDefaultCc).Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_EQ(reply[0], storages::redis::HexpireReply::kExpirationUpdated);
    }

    // Httl returns a positive value (in seconds) close to expected
    {
        auto reply = client->Httl(key, {"f1"}, kMasterCC).Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_TRUE(reply[0].KeyExists());
        EXPECT_TRUE(reply[0].KeyHasExpiration());
        EXPECT_GT(reply[0].GetExpire().count(), 0);
        EXPECT_LE(reply[0].GetExpire().count(), 60);
    }

    // Field without TTL: Httl reports no expiration
    {
        auto reply = client->Httl(key, {"f2"}, kMasterCC).Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_TRUE(reply[0].KeyExists());
        EXPECT_FALSE(reply[0].KeyHasExpiration());
    }

    // Missing field: Httl reports the field does not exist
    {
        auto reply = client->Httl(key, {"nope"}, kMasterCC).Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_FALSE(reply[0].KeyExists());
    }

    client->Del(key, kDefaultCc).Get();
}

UTEST_F(RedisClusterClientTest, HexpireBasicMs) {
    if (!HasHashExpireCommands()) {
        GTEST_SKIP() << SkipMsgHashExpireUnsupported("HPEXPIRE");
    }

    auto client = GetClient();
    const std::string key = "{hash_expire}:basic_ms";

    UASSERT_NO_THROW(client->Hset(key, "f1", "v1", kDefaultCc).Get());
    UASSERT_NO_THROW(client->Hset(key, "f2", "v2", kDefaultCc).Get());

    // Set TTL on existing field (ms-precision)
    {
        auto reply = client->Hpexpire(key, std::chrono::milliseconds(60000), {"f1"}, kDefaultCc).Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_EQ(reply[0], storages::redis::HexpireReply::kExpirationUpdated);
    }

    // Hpttl returns a positive value (in ms) close to expected
    {
        auto reply = client->Hpttl(key, {"f1"}, kMasterCC).Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_TRUE(reply[0].KeyExists());
        EXPECT_TRUE(reply[0].KeyHasExpiration());
        EXPECT_GT(reply[0].GetExpire().count(), 0);
        EXPECT_LE(reply[0].GetExpire().count(), 60000);
    }

    // Field without TTL: Hpttl reports no expiration
    {
        auto reply = client->Hpttl(key, {"f2"}, kMasterCC).Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_TRUE(reply[0].KeyExists());
        EXPECT_FALSE(reply[0].KeyHasExpiration());
    }

    // Missing field: Hpttl reports the field does not exist
    {
        auto reply = client->Hpttl(key, {"nope"}, kMasterCC).Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_FALSE(reply[0].KeyExists());
    }

    client->Del(key, kDefaultCc).Get();
}

UTEST_F(RedisClusterClientTest, HexpireConditionsSec) {
    if (!HasHashExpireCommands()) {
        GTEST_SKIP() << SkipMsgHashExpireUnsupported("HEXPIRE");
    }

    auto client = GetClient();
    const std::string key = "{hash_expire}:conditions_sec";

    UASSERT_NO_THROW(client->Hset(key, "with_ttl", "v1", kDefaultCc).Get());
    UASSERT_NO_THROW(client->Hset(key, "no_ttl", "v2", kDefaultCc).Get());

    UASSERT_NO_THROW(client->Hexpire(key, std::chrono::seconds(60), {"with_ttl"}, kDefaultCc).Get());

    // XX on a field that already has TTL => kExpirationUpdated
    {
        auto reply =
            client
                ->Hexpire(
                    key,
                    std::chrono::seconds(120),
                    storages::redis::ExpireOptions{storages::redis::ExpireOptions::Exist::kSetIfExist},
                    {"with_ttl"},
                    kDefaultCc
                )
                .Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_EQ(reply[0], storages::redis::HexpireReply::kExpirationUpdated);
    }

    // NX on a field that already has TTL => kConditionNotMet
    {
        auto reply =
            client
                ->Hexpire(
                    key,
                    std::chrono::seconds(120),
                    storages::redis::ExpireOptions{storages::redis::ExpireOptions::Exist::kSetIfNotExist},
                    {"with_ttl"},
                    kDefaultCc
                )
                .Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_EQ(reply[0], storages::redis::HexpireReply::kConditionNotMet);
    }

    // NX on a field with no TTL => kExpirationUpdated
    {
        auto reply =
            client
                ->Hexpire(
                    key,
                    std::chrono::seconds(120),
                    storages::redis::ExpireOptions{storages::redis::ExpireOptions::Exist::kSetIfNotExist},
                    {"no_ttl"},
                    kDefaultCc
                )
                .Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_EQ(reply[0], storages::redis::HexpireReply::kExpirationUpdated);
    }

    // GT with a larger ttl => kExpirationUpdated
    {
        auto reply =
            client
                ->Hexpire(
                    key,
                    std::chrono::seconds(600),
                    storages::redis::ExpireOptions{storages::redis::ExpireOptions::Compare::kGreaterThan},
                    {"with_ttl"},
                    kDefaultCc
                )
                .Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_EQ(reply[0], storages::redis::HexpireReply::kExpirationUpdated);
    }

    // GT with a smaller ttl => kConditionNotMet
    {
        auto reply =
            client
                ->Hexpire(
                    key,
                    std::chrono::seconds(10),
                    storages::redis::ExpireOptions{storages::redis::ExpireOptions::Compare::kGreaterThan},
                    {"with_ttl"},
                    kDefaultCc
                )
                .Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_EQ(reply[0], storages::redis::HexpireReply::kConditionNotMet);
    }

    // Missing field => kFieldDoesNotExist
    {
        auto reply =
            client
                ->Hexpire(
                    key,
                    std::chrono::seconds(60),
                    storages::redis::ExpireOptions{storages::redis::ExpireOptions::Exist::kSetIfExist},
                    {"missing"},
                    kDefaultCc
                )
                .Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_EQ(reply[0], storages::redis::HexpireReply::kFieldDoesNotExist);
    }

    client->Del(key, kDefaultCc).Get();
}

UTEST_F(RedisClusterClientTest, HexpireConditionsMs) {
    if (!HasHashExpireCommands()) {
        GTEST_SKIP() << SkipMsgHashExpireUnsupported("HPEXPIRE");
    }

    auto client = GetClient();
    const std::string key = "{hash_expire}:conditions_ms";

    UASSERT_NO_THROW(client->Hset(key, "with_ttl", "v1", kDefaultCc).Get());
    UASSERT_NO_THROW(client->Hset(key, "no_ttl", "v2", kDefaultCc).Get());

    UASSERT_NO_THROW(client->Hpexpire(key, std::chrono::milliseconds(60000), {"with_ttl"}, kDefaultCc).Get());

    // XX on a field that already has TTL => kExpirationUpdated
    {
        auto reply =
            client
                ->Hpexpire(
                    key,
                    std::chrono::milliseconds(120000),
                    storages::redis::ExpireOptions{storages::redis::ExpireOptions::Exist::kSetIfExist},
                    {"with_ttl"},
                    kDefaultCc
                )
                .Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_EQ(reply[0], storages::redis::HexpireReply::kExpirationUpdated);
    }

    // LT with a smaller ttl => kExpirationUpdated
    {
        auto reply =
            client
                ->Hpexpire(
                    key,
                    std::chrono::milliseconds(5000),
                    storages::redis::ExpireOptions{storages::redis::ExpireOptions::Compare::kLessThan},
                    {"with_ttl"},
                    kDefaultCc
                )
                .Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_EQ(reply[0], storages::redis::HexpireReply::kExpirationUpdated);
    }

    // LT with a larger ttl => kConditionNotMet
    {
        auto reply =
            client
                ->Hpexpire(
                    key,
                    std::chrono::milliseconds(9999000),
                    storages::redis::ExpireOptions{storages::redis::ExpireOptions::Compare::kLessThan},
                    {"with_ttl"},
                    kDefaultCc
                )
                .Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_EQ(reply[0], storages::redis::HexpireReply::kConditionNotMet);
    }

    client->Del(key, kDefaultCc).Get();
}

UTEST_F(RedisClusterClientTest, HexpireDeletesOnZeroSec) {
    if (!HasHashExpireCommands()) {
        GTEST_SKIP() << SkipMsgHashExpireUnsupported("HEXPIRE");
    }

    auto client = GetClient();
    const std::string key = "{hash_expire}:delete_on_zero_sec";

    UASSERT_NO_THROW(client->Hset(key, "f1", "v1", kDefaultCc).Get());

    {
        auto reply = client->Hexpire(key, std::chrono::seconds(0), {"f1"}, kDefaultCc).Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_EQ(reply[0], storages::redis::HexpireReply::kFieldDeleted);
    }

    {
        auto reply = client->Hget(key, "f1", kMasterCC).Get();
        EXPECT_FALSE(reply.has_value());
    }

    client->Del(key, kDefaultCc).Get();
}

UTEST_F(RedisClusterClientTest, HexpireDeletesOnZeroMs) {
    if (!HasHashExpireCommands()) {
        GTEST_SKIP() << SkipMsgHashExpireUnsupported("HPEXPIRE");
    }

    auto client = GetClient();
    const std::string key = "{hash_expire}:delete_on_zero_ms";

    UASSERT_NO_THROW(client->Hset(key, "f1", "v1", kDefaultCc).Get());

    {
        auto reply = client->Hpexpire(key, std::chrono::milliseconds(0), {"f1"}, kDefaultCc).Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_EQ(reply[0], storages::redis::HexpireReply::kFieldDeleted);
    }

    {
        auto reply = client->Hget(key, "f1", kMasterCC).Get();
        EXPECT_FALSE(reply.has_value());
    }

    client->Del(key, kDefaultCc).Get();
}

UTEST_F(RedisClusterClientTest, HexpireatBasicSec) {
    if (!HasHashExpireCommands()) {
        GTEST_SKIP() << SkipMsgHashExpireUnsupported("HEXPIREAT");
    }

    auto client = GetClient();
    const std::string key = "{hash_expire}:expireat_sec";

    UASSERT_NO_THROW(client->Hset(key, "f1", "v1", kDefaultCc).Get());

    const auto target = FutureDeadlineUtc();

    {
        auto reply = client->Hexpireat(key, target, {"f1"}, kDefaultCc).Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_EQ(reply[0], storages::redis::HexpireReply::kExpirationUpdated);
    }

    {
        auto reply = client->Hexpiretime(key, {"f1"}, kMasterCC).Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_TRUE(reply[0].FieldExists());
        ASSERT_TRUE(reply[0].HasExpiration());
        EXPECT_EQ(reply[0].GetDeadline(), target);
    }

    client->Del(key, kDefaultCc).Get();
}

UTEST_F(RedisClusterClientTest, HexpireatBasicMs) {
    if (!HasHashExpireCommands()) {
        GTEST_SKIP() << SkipMsgHashExpireUnsupported("HPEXPIREAT");
    }

    auto client = GetClient();
    const std::string key = "{hash_expire}:expireat_ms";

    UASSERT_NO_THROW(client->Hset(key, "f1", "v1", kDefaultCc).Get());

    const auto target = FutureDeadlineUtcWithMilliseconds();

    {
        auto reply = client->Hpexpireat(key, target, {"f1"}, kDefaultCc).Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_EQ(reply[0], storages::redis::HexpireReply::kExpirationUpdated);
    }

    {
        auto reply = client->Hpexpiretime(key, {"f1"}, kMasterCC).Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_TRUE(reply[0].FieldExists());
        ASSERT_TRUE(reply[0].HasExpiration());
        EXPECT_EQ(reply[0].GetDeadline(), target);
    }

    client->Del(key, kDefaultCc).Get();
}

UTEST_F(RedisClusterClientTest, Hpersist) {
    if (!HasHashExpireCommands()) {
        GTEST_SKIP() << SkipMsgHashExpireUnsupported("HPERSIST");
    }

    auto client = GetClient();
    const std::string key = "{hash_expire}:persist";

    UASSERT_NO_THROW(client->Hset(key, "with_ttl", "v1", kDefaultCc).Get());
    UASSERT_NO_THROW(client->Hset(key, "no_ttl", "v2", kDefaultCc).Get());
    UASSERT_NO_THROW(client->Hexpire(key, std::chrono::seconds(60), {"with_ttl"}, kDefaultCc).Get());

    {
        auto reply = client->Hpersist(key, {"with_ttl"}, kDefaultCc).Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_EQ(reply[0], storages::redis::HpersistReply::kExpirationRemoved);
    }

    {
        auto reply = client->Httl(key, {"with_ttl"}, kMasterCC).Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_TRUE(reply[0].KeyExists());
        EXPECT_FALSE(reply[0].KeyHasExpiration());
    }

    {
        auto reply = client->Hpersist(key, {"no_ttl"}, kDefaultCc).Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_EQ(reply[0], storages::redis::HpersistReply::kFieldHasNoExpiration);
    }

    {
        auto reply = client->Hpersist(key, {"missing"}, kDefaultCc).Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_EQ(reply[0], storages::redis::HpersistReply::kFieldDoesNotExist);
    }

    client->Del(key, kDefaultCc).Get();
}

UTEST_F(RedisClusterClientTest, HgetexPlain) {
    if (!HasHashExpireCommands()) {
        GTEST_SKIP() << SkipMsgHashExpireUnsupported("HGETEX");
    }

    auto client = GetClient();
    const std::string key = "{hash_expire}:getex_plain";

    UASSERT_NO_THROW(client->Hmset(key, {{"f1", "v1"}, {"f2", "v2"}}, kDefaultCc).Get());
    UASSERT_NO_THROW(client->Hexpire(key, std::chrono::seconds(120), {"f1", "f2"}, kDefaultCc).Get());

    {
        auto reply = client->Hgetex(key, {"f1", "f2", "missing"}, kMasterCC).Get();
        ASSERT_EQ(reply.size(), 3U);
        ASSERT_TRUE(reply[0].has_value());
        EXPECT_EQ(reply[0].value(), "v1");
        ASSERT_TRUE(reply[1].has_value());
        EXPECT_EQ(reply[1].value(), "v2");
        EXPECT_FALSE(reply[2].has_value());
    }

    // TTL should be unchanged
    {
        auto reply = client->Httl(key, {"f1", "f2"}, kMasterCC).Get();
        ASSERT_EQ(reply.size(), 2U);
        EXPECT_TRUE(reply[0].KeyHasExpiration());
        EXPECT_GT(reply[0].GetExpire().count(), 0);
        EXPECT_LE(reply[0].GetExpire().count(), 120);
        EXPECT_TRUE(reply[1].KeyHasExpiration());
        EXPECT_GT(reply[1].GetExpire().count(), 0);
        EXPECT_LE(reply[1].GetExpire().count(), 120);
    }

    client->Del(key, kDefaultCc).Get();
}

UTEST_F(RedisClusterClientTest, HgetexWithExpire) {
    if (!HasHashExpireCommands()) {
        GTEST_SKIP() << SkipMsgHashExpireUnsupported("HGETEX");
    }

    auto client = GetClient();
    const std::string key = "{hash_expire}:getex_ex";

    UASSERT_NO_THROW(client->Hmset(key, {{"f1", "v1"}, {"f2", "v2"}}, kDefaultCc).Get());

    {
        auto reply =
            client
                ->Hgetex(
                    key,
                    storages::redis::HgetexOptions::Expire(std::chrono::milliseconds(60000)),
                    {"f1", "f2"},
                    kDefaultCc
                )
                .Get();
        ASSERT_EQ(reply.size(), 2U);
        ASSERT_TRUE(reply[0].has_value());
        EXPECT_EQ(reply[0].value(), "v1");
        ASSERT_TRUE(reply[1].has_value());
        EXPECT_EQ(reply[1].value(), "v2");
    }

    {
        auto reply = client->Hpttl(key, {"f1", "f2"}, kMasterCC).Get();
        ASSERT_EQ(reply.size(), 2U);
        EXPECT_TRUE(reply[0].KeyHasExpiration());
        EXPECT_GT(reply[0].GetExpire().count(), 0);
        EXPECT_LE(reply[0].GetExpire().count(), 60000);
        EXPECT_TRUE(reply[1].KeyHasExpiration());
        EXPECT_GT(reply[1].GetExpire().count(), 0);
        EXPECT_LE(reply[1].GetExpire().count(), 60000);
    }

    client->Del(key, kDefaultCc).Get();
}

UTEST_F(RedisClusterClientTest, HgetexWithPersist) {
    if (!HasHashExpireCommands()) {
        GTEST_SKIP() << SkipMsgHashExpireUnsupported("HGETEX");
    }

    auto client = GetClient();
    const std::string key = "{hash_expire}:getex_persist";

    UASSERT_NO_THROW(client->Hmset(key, {{"f1", "v1"}, {"f2", "v2"}}, kDefaultCc).Get());
    UASSERT_NO_THROW(client->Hexpire(key, std::chrono::seconds(120), {"f1", "f2"}, kDefaultCc).Get());

    {
        auto reply = client->Hgetex(key, storages::redis::HgetexOptions::Persist(), {"f1", "f2"}, kDefaultCc).Get();
        ASSERT_EQ(reply.size(), 2U);
        ASSERT_TRUE(reply[0].has_value());
        EXPECT_EQ(reply[0].value(), "v1");
        ASSERT_TRUE(reply[1].has_value());
        EXPECT_EQ(reply[1].value(), "v2");
    }

    {
        auto reply = client->Httl(key, {"f1", "f2"}, kMasterCC).Get();
        ASSERT_EQ(reply.size(), 2U);
        EXPECT_TRUE(reply[0].KeyExists());
        EXPECT_FALSE(reply[0].KeyHasExpiration());
        EXPECT_TRUE(reply[1].KeyExists());
        EXPECT_FALSE(reply[1].KeyHasExpiration());
    }

    client->Del(key, kDefaultCc).Get();
}

UTEST_F(RedisClusterClientTest, HgetexExAt) {
    if (!HasHashExpireCommands()) {
        GTEST_SKIP() << SkipMsgHashExpireUnsupported("HGETEX");
    }

    auto client = GetClient();
    const std::string key = "{hash_expire}:getex_exat";

    UASSERT_NO_THROW(client->Hset(key, "f1", "v1", kDefaultCc).Get());

    const auto target = FutureDeadlineUtcWithMilliseconds();

    {
        auto reply = client->Hgetex(key, storages::redis::HgetexOptions::ExpireAt(target), {"f1"}, kDefaultCc).Get();
        ASSERT_EQ(reply.size(), 1U);
        ASSERT_TRUE(reply[0].has_value());
        EXPECT_EQ(reply[0].value(), "v1");
    }

    {
        auto reply = client->Hpexpiretime(key, {"f1"}, kMasterCC).Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_TRUE(reply[0].FieldExists());
        ASSERT_TRUE(reply[0].HasExpiration());
        EXPECT_EQ(reply[0].GetDeadline(), target);
    }

    client->Del(key, kDefaultCc).Get();
}

UTEST_F(RedisClusterClientTest, HsetexBasic) {
    if (!HasHashExpireCommands()) {
        GTEST_SKIP() << SkipMsgHashExpireUnsupported("HSETEX");
    }

    auto client = GetClient();
    const std::string key = "{hash_expire}:setex_basic";

    {
        std::vector<storages::redis::HsetexFieldValue> fvs{{"f1", "v1"}, {"f2", "v2"}};
        auto reply =
            client
                ->Hsetex(
                    key,
                    storages::redis::HsetexOptions::Expire(std::chrono::milliseconds(60000)),
                    std::move(fvs),
                    kDefaultCc
                )
                .Get();
        EXPECT_EQ(reply, storages::redis::HsetexReply::kFieldsSet);
    }

    {
        auto reply = client->Hget(key, "f1", kMasterCC).Get();
        ASSERT_TRUE(reply.has_value());
        EXPECT_EQ(reply.value(), "v1");
    }

    {
        auto reply = client->Hpttl(key, {"f1", "f2"}, kMasterCC).Get();
        ASSERT_EQ(reply.size(), 2U);
        EXPECT_TRUE(reply[0].KeyHasExpiration());
        EXPECT_GT(reply[0].GetExpire().count(), 0);
        EXPECT_LE(reply[0].GetExpire().count(), 60000);
        EXPECT_TRUE(reply[1].KeyHasExpiration());
        EXPECT_GT(reply[1].GetExpire().count(), 0);
        EXPECT_LE(reply[1].GetExpire().count(), 60000);
    }

    client->Del(key, kDefaultCc).Get();
}

UTEST_F(RedisClusterClientTest, HsetexFnxFxx) {
    if (!HasHashExpireCommands()) {
        GTEST_SKIP() << SkipMsgHashExpireUnsupported("HSETEX");
    }

    auto client = GetClient();
    const std::string key = "{hash_expire}:setex_fnx_fxx";

    // FNX on non-existing field => 1
    {
        std::vector<storages::redis::HsetexFieldValue> fvs{{"f1", "v1"}};
        auto reply =
            client
                ->Hsetex(
                    key,
                    storages::redis::HsetexOptions::Expire(std::chrono::milliseconds(60000)).OnlyIfNoneOfFieldsExist(),
                    std::move(fvs),
                    kDefaultCc
                )
                .Get();
        EXPECT_EQ(reply, storages::redis::HsetexReply::kFieldsSet);
    }

    // FNX on existing field => 0
    {
        std::vector<storages::redis::HsetexFieldValue> fvs{{"f1", "v1b"}};
        auto reply =
            client
                ->Hsetex(
                    key,
                    storages::redis::HsetexOptions::Expire(std::chrono::milliseconds(60000)).OnlyIfNoneOfFieldsExist(),
                    std::move(fvs),
                    kDefaultCc
                )
                .Get();
        EXPECT_EQ(reply, storages::redis::HsetexReply::kConditionNotMet);
    }

    // FXX on existing field => 1
    {
        std::vector<storages::redis::HsetexFieldValue> fvs{{"f1", "v1c"}};
        auto reply =
            client
                ->Hsetex(
                    key,
                    storages::redis::HsetexOptions::Expire(std::chrono::milliseconds(60000)).OnlyIfAllFieldsExist(),
                    std::move(fvs),
                    kDefaultCc
                )
                .Get();
        EXPECT_EQ(reply, storages::redis::HsetexReply::kFieldsSet);
    }

    // FXX on non-existing field => 0
    {
        std::vector<storages::redis::HsetexFieldValue> fvs{{"new_field", "new_value"}};
        auto reply =
            client
                ->Hsetex(
                    key,
                    storages::redis::HsetexOptions::Expire(std::chrono::milliseconds(60000)).OnlyIfAllFieldsExist(),
                    std::move(fvs),
                    kDefaultCc
                )
                .Get();
        EXPECT_EQ(reply, storages::redis::HsetexReply::kConditionNotMet);
    }

    client->Del(key, kDefaultCc).Get();
}

UTEST_F(RedisClusterClientTest, HsetexKeepTtl) {
    if (!HasHashExpireCommands()) {
        GTEST_SKIP() << SkipMsgHashExpireUnsupported("HSETEX");
    }

    auto client = GetClient();
    const std::string key = "{hash_expire}:setex_keepttl";

    // Initial HSETEX with 60s TTL
    {
        std::vector<storages::redis::HsetexFieldValue> fvs{{"f1", "v1"}};
        auto reply =
            client
                ->Hsetex(
                    key,
                    storages::redis::HsetexOptions::Expire(std::chrono::milliseconds(60000)),
                    std::move(fvs),
                    kDefaultCc
                )
                .Get();
        EXPECT_EQ(reply, storages::redis::HsetexReply::kFieldsSet);
    }

    // Second HSETEX with KEEPTTL — base seconds should be ignored
    {
        std::vector<storages::redis::HsetexFieldValue> fvs{{"f1", "v1_updated"}};
        auto reply = client->Hsetex(key, storages::redis::HsetexOptions::KeepTtl(), std::move(fvs), kDefaultCc).Get();
        EXPECT_EQ(reply, storages::redis::HsetexReply::kFieldsSet);
    }

    // TTL should be roughly the original 60s
    {
        auto reply = client->Httl(key, {"f1"}, kMasterCC).Get();
        ASSERT_EQ(reply.size(), 1U);
        EXPECT_TRUE(reply[0].KeyHasExpiration());
        EXPECT_GT(reply[0].GetExpire().count(), 0);
        EXPECT_LE(reply[0].GetExpire().count(), 60);
    }

    {
        auto reply = client->Hget(key, "f1", kMasterCC).Get();
        ASSERT_TRUE(reply.has_value());
        EXPECT_EQ(reply.value(), "v1_updated");
    }

    client->Del(key, kDefaultCc).Get();
}

UTEST_F(RedisClusterClientTest, HexpireMultipleFieldsSec) {
    if (!HasHashExpireCommands()) {
        GTEST_SKIP() << SkipMsgHashExpireUnsupported("HEXPIRE");
    }

    auto client = GetClient();
    const std::string key = "{hash_expire}:multi_sec";

    UASSERT_NO_THROW(client->Hmset(key, {{"f1", "v1"}, {"f2", "v2"}, {"f3", "v3"}}, kDefaultCc).Get());

    auto reply = client->Hexpire(key, std::chrono::seconds(60), {"f1", "f2", "f3"}, kDefaultCc).Get();
    ASSERT_EQ(reply.size(), 3U);
    EXPECT_EQ(reply[0], storages::redis::HexpireReply::kExpirationUpdated);
    EXPECT_EQ(reply[1], storages::redis::HexpireReply::kExpirationUpdated);
    EXPECT_EQ(reply[2], storages::redis::HexpireReply::kExpirationUpdated);

    client->Del(key, kDefaultCc).Get();
}

UTEST_F(RedisClusterClientTest, HexpireMultipleFieldsMs) {
    if (!HasHashExpireCommands()) {
        GTEST_SKIP() << SkipMsgHashExpireUnsupported("HPEXPIRE");
    }

    auto client = GetClient();
    const std::string key = "{hash_expire}:multi_ms";

    UASSERT_NO_THROW(client->Hmset(key, {{"f1", "v1"}, {"f2", "v2"}, {"f3", "v3"}}, kDefaultCc).Get());

    auto reply = client->Hpexpire(key, std::chrono::milliseconds(60000), {"f1", "f2", "f3"}, kDefaultCc).Get();
    ASSERT_EQ(reply.size(), 3U);
    EXPECT_EQ(reply[0], storages::redis::HexpireReply::kExpirationUpdated);
    EXPECT_EQ(reply[1], storages::redis::HexpireReply::kExpirationUpdated);
    EXPECT_EQ(reply[2], storages::redis::HexpireReply::kExpirationUpdated);

    client->Del(key, kDefaultCc).Get();
}

UTEST_F(RedisClusterClientTest, HsetexMultipleFieldValues) {
    if (!HasHashExpireCommands()) {
        GTEST_SKIP() << SkipMsgHashExpireUnsupported("HSETEX");
    }

    auto client = GetClient();
    const std::string key = "{hash_expire}:setex_multi";

    {
        std::vector<storages::redis::HsetexFieldValue> fvs{
            {"f1", "v1"},
            {"f2", "v2"},
            {"f3", "v3"},
        };
        auto reply =
            client
                ->Hsetex(
                    key,
                    storages::redis::HsetexOptions::Expire(std::chrono::milliseconds(60000)),
                    std::move(fvs),
                    kDefaultCc
                )
                .Get();
        EXPECT_EQ(reply, storages::redis::HsetexReply::kFieldsSet);
    }

    auto all = client->Hgetall(key, kMasterCC).Get();
    ASSERT_EQ(all.size(), 3U);
    EXPECT_EQ(all["f1"], "v1");
    EXPECT_EQ(all["f2"], "v2");
    EXPECT_EQ(all["f3"], "v3");

    client->Del(key, kDefaultCc).Get();
}

UTEST_F(RedisClusterClientTest, TransactionHexpireSec) {
    if (!HasHashExpireCommands()) {
        GTEST_SKIP() << SkipMsgHashExpireUnsupported("HEXPIRE");
    }

    auto client = GetClient();
    const std::string key = "{hash_expire}:tx_sec";

    auto transaction = client->Multi();

    auto set_req = transaction->Hset(key, "f1", "v1");
    auto exp_req = transaction->Hexpire(key, std::chrono::seconds(60), {"f1"});
    auto ttl_req = transaction->Httl(key, {"f1"});

    UASSERT_NO_THROW(transaction->Exec(kDefaultCc).Get());

    UEXPECT_NO_THROW(set_req.Get());

    auto exp_reply = exp_req.Get();
    ASSERT_EQ(exp_reply.size(), 1U);
    EXPECT_EQ(exp_reply[0], storages::redis::HexpireReply::kExpirationUpdated);

    auto ttl_reply = ttl_req.Get();
    ASSERT_EQ(ttl_reply.size(), 1U);
    EXPECT_TRUE(ttl_reply[0].KeyHasExpiration());
    EXPECT_GT(ttl_reply[0].GetExpire().count(), 0);
    EXPECT_LE(ttl_reply[0].GetExpire().count(), 60);

    client->Del(key, kDefaultCc).Get();
}

UTEST_F(RedisClusterClientTest, TransactionHexpireMs) {
    if (!HasHashExpireCommands()) {
        GTEST_SKIP() << SkipMsgHashExpireUnsupported("HPEXPIRE");
    }

    auto client = GetClient();
    const std::string key = "{hash_expire}:tx_ms";

    auto transaction = client->Multi();

    auto set_req = transaction->Hset(key, "f1", "v1");
    auto exp_req = transaction->Hpexpire(key, std::chrono::milliseconds(60000), {"f1"});
    auto ttl_req = transaction->Hpttl(key, {"f1"});

    UASSERT_NO_THROW(transaction->Exec(kDefaultCc).Get());

    UEXPECT_NO_THROW(set_req.Get());

    auto exp_reply = exp_req.Get();
    ASSERT_EQ(exp_reply.size(), 1U);
    EXPECT_EQ(exp_reply[0], storages::redis::HexpireReply::kExpirationUpdated);

    auto ttl_reply = ttl_req.Get();
    ASSERT_EQ(ttl_reply.size(), 1U);
    EXPECT_TRUE(ttl_reply[0].KeyHasExpiration());
    EXPECT_GT(ttl_reply[0].GetExpire().count(), 0);
    EXPECT_LE(ttl_reply[0].GetExpire().count(), 60000);

    client->Del(key, kDefaultCc).Get();
}

USERVER_NAMESPACE_END
