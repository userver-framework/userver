#include <storages/redis/impl/keyshard_impl.hpp>
#include <storages/redis/impl/sentinel.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

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

    const auto fail_pmessage =
        [](storages::redis::ServerId, const std::string&, const std::string&, const std::string&) {
            FAIL() << "pmessage callback must not fire on empty array";
        };
    const auto fail_subscribe = [](storages::redis::ServerId, const std::string&, size_t) {
        FAIL() << "subscribe callback must not fire on empty array";
    };
    const auto fail_unsubscribe = [](storages::redis::ServerId, const std::string&, size_t) {
        FAIL() << "unsubscribe callback must not fire on empty array";
    };

    Sentinel::OnPsubscribeReply(fail_pmessage, fail_subscribe, fail_unsubscribe, reply);
}

TEST(Sentinel, OnPsubscribeReplyTooShortArray) {
    using storages::redis::Reply;
    using storages::redis::ReplyData;
    using storages::redis::impl::Sentinel;

    const auto fail_pmessage =
        [](storages::redis::ServerId, const std::string&, const std::string&, const std::string&) {
            FAIL() << "pmessage callback must not fire on malformed array";
        };
    const auto fail_subscribe = [](storages::redis::ServerId, const std::string&, size_t) {
        FAIL() << "subscribe callback must not fire on malformed array";
    };
    const auto fail_unsubscribe = [](storages::redis::ServerId, const std::string&, size_t) {
        FAIL() << "unsubscribe callback must not fire on malformed array";
    };

    // A server/proxy may answer with a well-formed array that is missing the
    // channel/count fields. Each of these must be ignored, not indexed OOB.
    const auto make_array = [](std::vector<std::string> parts) {
        ReplyData::Array array;
        for (auto& part : parts) {
            array.emplace_back(std::move(part));
        }
        return array;
    };

    for (auto& parts : std::vector<std::vector<std::string>>{
             {"PSUBSCRIBE"},                       // command only, no channel/count
             {"PSUBSCRIBE", "news.*"},             // missing count
             {"PUNSUBSCRIBE"},                     // command only
             {"PUNSUBSCRIBE", "news.*"},           // missing count
             {"PMESSAGE"},                         // command only
             {"PMESSAGE", "news.*"},               // missing channel/payload
             {"PMESSAGE", "news.*", "news.tech"},  // missing payload
         })
    {
        auto reply = std::make_shared<Reply>("PSUBSCRIBE", ReplyData{make_array(std::move(parts))});
        Sentinel::OnPsubscribeReply(fail_pmessage, fail_subscribe, fail_unsubscribe, reply);
    }
}

namespace {

// A malicious/compromised server may answer with a well-formed array of the
// expected length whose elements have unexpected types. ReplyData::GetString()/
// GetInt() only UASSERT the type (a no-op in release) and then dereference the
// pointer returned by std::get_if, which is null on a type mismatch. So each of
// these replies must be ignored, not passed to a typed accessor.
[[nodiscard]] storages::redis::ReplyData::Array MakeReplyArray(std::vector<storages::redis::ReplyData> parts) {
    storages::redis::ReplyData::Array array;
    for (auto& part : parts) {
        array.push_back(std::move(part));
    }
    return array;
}

}  // namespace

TEST(Sentinel, OnSubscribeReplyWrongElementTypes) {
    using storages::redis::Reply;
    using storages::redis::ReplyData;
    using storages::redis::impl::Sentinel;

    const auto fail_message = [](storages::redis::ServerId, const std::string&, const std::string&) {
        FAIL() << "message callback must not fire on a wrong-typed reply element";
    };
    const auto fail_subscribe = [](storages::redis::ServerId, const std::string&, size_t) {
        FAIL() << "subscribe callback must not fire on a wrong-typed reply element";
    };
    const auto fail_unsubscribe = [](storages::redis::ServerId, const std::string&, size_t) {
        FAIL() << "unsubscribe callback must not fire on a wrong-typed reply element";
    };

    std::vector<ReplyData::Array> replies;
    // channel is expected to be a string, count an integer
    replies.push_back(MakeReplyArray({std::string{"SUBSCRIBE"}, ReplyData{42}, ReplyData{1}}));
    replies.push_back(MakeReplyArray({std::string{"SUBSCRIBE"}, std::string{"news"}, std::string{"1"}}));
    replies.push_back(MakeReplyArray({std::string{"UNSUBSCRIBE"}, ReplyData{42}, ReplyData{0}}));
    replies.push_back(MakeReplyArray({std::string{"UNSUBSCRIBE"}, std::string{"news"}, std::string{"0"}}));
    // channel and message are both expected to be strings
    replies.push_back(MakeReplyArray({std::string{"MESSAGE"}, ReplyData{42}, std::string{"payload"}}));
    replies.push_back(MakeReplyArray({std::string{"MESSAGE"}, std::string{"news"}, ReplyData{42}}));

    for (auto& array : replies) {
        auto reply = std::make_shared<Reply>("SUBSCRIBE", ReplyData{std::move(array)});
        Sentinel::OnSubscribeReply(fail_message, fail_subscribe, fail_unsubscribe, reply);
    }

    // The sharded pub/sub variant shares the same dispatch (different opcodes).
    std::vector<ReplyData::Array> sharded_replies;
    sharded_replies.push_back(MakeReplyArray({std::string{"SSUBSCRIBE"}, ReplyData{42}, ReplyData{1}}));
    sharded_replies.push_back(MakeReplyArray({std::string{"SUNSUBSCRIBE"}, ReplyData{42}, ReplyData{0}}));
    sharded_replies.push_back(MakeReplyArray({std::string{"SMESSAGE"}, ReplyData{42}, std::string{"payload"}}));
    sharded_replies.push_back(MakeReplyArray({std::string{"SMESSAGE"}, std::string{"news"}, ReplyData{42}}));
    for (auto& array : sharded_replies) {
        auto reply = std::make_shared<Reply>("SSUBSCRIBE", ReplyData{std::move(array)});
        Sentinel::OnSsubscribeReply(fail_message, fail_subscribe, fail_unsubscribe, reply);
    }
}

TEST(Sentinel, OnPsubscribeReplyWrongElementTypes) {
    using storages::redis::Reply;
    using storages::redis::ReplyData;
    using storages::redis::impl::Sentinel;

    const auto fail_pmessage =
        [](storages::redis::ServerId, const std::string&, const std::string&, const std::string&) {
            FAIL() << "pmessage callback must not fire on a wrong-typed reply element";
        };
    const auto fail_subscribe = [](storages::redis::ServerId, const std::string&, size_t) {
        FAIL() << "subscribe callback must not fire on a wrong-typed reply element";
    };
    const auto fail_unsubscribe = [](storages::redis::ServerId, const std::string&, size_t) {
        FAIL() << "unsubscribe callback must not fire on a wrong-typed reply element";
    };

    std::vector<ReplyData::Array> replies;
    replies.push_back(MakeReplyArray({std::string{"PSUBSCRIBE"}, ReplyData{42}, ReplyData{1}}));
    replies.push_back(MakeReplyArray({std::string{"PSUBSCRIBE"}, std::string{"news.*"}, std::string{"1"}}));
    replies.push_back(MakeReplyArray({std::string{"PUNSUBSCRIBE"}, ReplyData{42}, ReplyData{0}}));
    replies.push_back(MakeReplyArray({std::string{"PUNSUBSCRIBE"}, std::string{"news.*"}, std::string{"0"}}));
    // pattern, channel and message are all expected to be strings
    const std::string pat{"news.*"};
    const std::string chan{"news"};
    const std::string msg{"m"};
    replies.push_back(MakeReplyArray({std::string{"PMESSAGE"}, ReplyData{42}, chan, msg}));
    replies.push_back(MakeReplyArray({std::string{"PMESSAGE"}, pat, ReplyData{42}, msg}));
    replies.push_back(MakeReplyArray({std::string{"PMESSAGE"}, pat, chan, ReplyData{42}}));

    for (auto& array : replies) {
        auto reply = std::make_shared<Reply>("PSUBSCRIBE", ReplyData{std::move(array)});
        Sentinel::OnPsubscribeReply(fail_pmessage, fail_subscribe, fail_unsubscribe, reply);
    }
}

USERVER_NAMESPACE_END
