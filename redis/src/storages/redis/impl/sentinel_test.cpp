#include <storages/redis/impl/keyshard_impl.hpp>
#include <storages/redis/impl/sentinel.hpp>

#include <memory>

#include <gtest/gtest.h>

#include <userver/storages/redis/reply.hpp>

USERVER_NAMESPACE_BEGIN

TEST(Sentinel, CreateTmpKey) {
    const storages::redis::impl::KeyShardCrc32 key_shard(0xffffffff);
    for (const char* const key : {"hello:world", "abc", "duke:nukem{must:die}"}) {
        const std::string& tmpkey = storages::redis::impl::Sentinel::CreateTmpKey(key);
        EXPECT_STRNE(key, tmpkey.c_str()) << key << " vs " << tmpkey;
        EXPECT_EQ(key_shard.ShardByKey(key), key_shard.ShardByKey(tmpkey)) << key << " vs " << tmpkey;
    }
}

TEST(Sentinel, OnPsubscribeReplyEmptyArray) {
    using storages::redis::Reply;
    using storages::redis::ReplyData;
    using storages::redis::impl::Sentinel;

    auto reply = std::make_shared<Reply>("PSUBSCRIBE", ReplyData{ReplyData::Array{}});
    ASSERT_TRUE(reply->data.IsArray());
    ASSERT_TRUE(reply->data.GetArray().empty());

    const auto fail_pmessage = [](storages::redis::ServerId, const std::string&, const std::string&,
                                  const std::string&) { FAIL() << "pmessage callback must not fire on empty array"; };
    const auto fail_subscribe = [](storages::redis::ServerId, const std::string&, size_t) {
        FAIL() << "subscribe callback must not fire on empty array";
    };
    const auto fail_unsubscribe = [](storages::redis::ServerId, const std::string&, size_t) {
        FAIL() << "unsubscribe callback must not fire on empty array";
    };

    Sentinel::OnPsubscribeReply(fail_pmessage, fail_subscribe, fail_unsubscribe, reply);
}

USERVER_NAMESPACE_END
