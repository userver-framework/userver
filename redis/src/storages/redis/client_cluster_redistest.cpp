#include <storages/redis/client_cluster_redistest.hpp>

#include <userver/engine/deadline.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>

#include <storages/redis/impl/cluster_sentinel_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const std::string kKeyNamePrefix = "test_key_";

std::string MakeKey(size_t idx) { return kKeyNamePrefix + std::to_string(idx); }

std::string MakeKey2(size_t idx, int add) {
    return "{" + MakeKey(idx) + "}not_hashed_suffix_" + std::to_string(add - idx);
}

constexpr storages::redis::CommandControl kDefaultCc{std::chrono::milliseconds(300), std::chrono::milliseconds(300), 1};

}  // namespace

UTEST_F(RedisClusterClientTest, SetGet) {
    auto client = GetClient();

    const size_t kNumKeys = 10;
    const int add = 100;

    for (size_t i = 0; i < kNumKeys; ++i) {
        auto req = client->Set(MakeKey(i), std::to_string(add + i), kDefaultCc);
        UASSERT_NO_THROW(req.Get());
    }

    for (size_t i = 0; i < kNumKeys; ++i) {
        auto req = client->Get(MakeKey(i), kDefaultCc);
        auto reply = req.Get();
        ASSERT_TRUE(reply);
        EXPECT_EQ(*reply, std::to_string(add + i));
    }

    for (size_t i = 0; i < kNumKeys; ++i) {
        auto req = client->Del(MakeKey(i), kDefaultCc);
        EXPECT_EQ(req.Get(), 1);
    }
}

UTEST_F(RedisClusterClientTest, Mget) {
    auto client = GetClient();

    const size_t kNumKeys = 10;
    const int add = 100;

    for (size_t i = 0; i < kNumKeys; ++i) {
        auto req = client->Set(MakeKey(i), std::to_string(add + i), kDefaultCc);
        UASSERT_NO_THROW(req.Get());
    }

    for (size_t i = 0; i < kNumKeys; ++i) {
        auto req = client->Set(MakeKey2(i, add), std::to_string(add * 2 + i), kDefaultCc);
        UASSERT_NO_THROW(req.Get());
    }

    for (size_t i = 0; i < kNumKeys; ++i) {
        auto req = client->Mget({MakeKey(i), MakeKey2(i, add)}, kDefaultCc);
        auto reply = req.Get();
        ASSERT_EQ(reply.size(), 2);

        ASSERT_TRUE(reply[0]);
        EXPECT_EQ(*reply[0], std::to_string(add + i));
        ASSERT_TRUE(reply[1]);
        EXPECT_EQ(*reply[1], std::to_string(add * 2 + i));
    }

    for (size_t i = 0; i < kNumKeys; ++i) {
        auto req = client->Del(MakeKey(i), kDefaultCc);
        EXPECT_EQ(req.Get(), 1);
    }

    for (size_t i = 0; i < kNumKeys; ++i) {
        auto req = client->Del(MakeKey2(i, add), kDefaultCc);
        EXPECT_EQ(req.Get(), 1);
    }
}

UTEST_F(RedisClusterClientTest, MgetCrossSlot) {
    auto client = GetClient();

    const int add = 100;

    size_t idx[2] = {0, 1};
    auto shard = client->ShardByKey(MakeKey(idx[0]));
    while (client->ShardByKey(MakeKey(idx[1])) != shard) ++idx[1];

    for (const unsigned long i : idx) {
        auto req = client->Set(MakeKey(i), std::to_string(add + i), kDefaultCc);
        UASSERT_NO_THROW(req.Get());
    }

    {
        auto req = client->Mget({MakeKey(idx[0]), MakeKey(idx[1])}, kDefaultCc);
        UASSERT_THROW(req.Get(), storages::redis::RequestFailedException);
    }

    for (const unsigned long i : idx) {
        auto req = client->Del(MakeKey(i), kDefaultCc);
        EXPECT_EQ(req.Get(), 1);
    }
}

UTEST_F(RedisClusterClientTest, Transaction) {
    auto client = GetClient();
    auto transaction = client->Multi();

    const int add = 100;

    auto set1 = transaction->Set(MakeKey(0), std::to_string(add));
    auto get1 = transaction->Get(MakeKey(0));
    auto set2 = transaction->Set(MakeKey2(0, add), std::to_string(add + 1));
    auto get2 = transaction->Get(MakeKey2(0, add));

    UASSERT_NO_THROW(transaction->Exec(kDefaultCc).Get());
    auto reply1 = get1.Get();
    ASSERT_TRUE(reply1);
    EXPECT_EQ(*reply1, std::to_string(add));
    auto reply2 = get2.Get();
    ASSERT_TRUE(reply2);
    EXPECT_EQ(*reply2, std::to_string(add + 1));

    {
        auto req = client->Del(MakeKey(0), kDefaultCc);
        EXPECT_EQ(req.Get(), 1);
    }
    {
        auto req = client->Del(MakeKey2(0, add), kDefaultCc);
        EXPECT_EQ(req.Get(), 1);
    }
}

UTEST_F(RedisClusterClientTest, TransactionSmokeRetriesFailure) {
    auto client = GetClient();
    using namespace std::chrono_literals;
    storages::redis::CommandControl kRetryCc{1ms, 300ms, 100};
    kRetryCc.allow_reads_from_master = true;

    const size_t kNumKeys = 3;
    const size_t kSubseqChanges = 1000;
    for (size_t i = 0; i < kNumKeys; ++i) {
        auto transaction = client->Multi();
        const auto key = MakeKey(i);
        for (size_t j = 0; j < kSubseqChanges; ++j) {
            [[maybe_unused]] auto set = transaction->Set(key, "some value" + std::to_string(j), 500ms);
            [[maybe_unused]] auto get = transaction->Get(key);
        }
        UASSERT_THROW(transaction->Exec(kRetryCc).Get(), storages::redis::RequestFailedException);
    }
}

UTEST_F(RedisClusterClientTest, TransactionCrossSlot) {
    auto client = GetClient();
    auto transaction = client->Multi();

    const int add = 100;

    size_t idx[2] = {0, 1};
    auto shard = client->ShardByKey(MakeKey(idx[0]));
    while (client->ShardByKey(MakeKey(idx[1])) != shard) ++idx[1];

    for (size_t i = 0; i < 2; ++i) {
        auto set = transaction->Set(MakeKey(idx[i]), std::to_string(add + i));
        auto get = transaction->Get(MakeKey(idx[i]));
    }
    UASSERT_THROW(transaction->Exec(kDefaultCc).Get(), storages::redis::RequestFailedException);
}

UTEST_F(RedisClusterClientTest, TransactionDistinctShards) {
    auto client = GetClient();
    auto transaction = client->Multi(storages::redis::Transaction::CheckShards::kNo);

    const size_t kNumKeys = 10;
    const int add = 100;

    for (size_t i = 0; i < kNumKeys; ++i) {
        auto set = transaction->Set(MakeKey(i), std::to_string(add + i));
        auto get = transaction->Get(MakeKey(i));
    }
    UASSERT_THROW(transaction->Exec(kDefaultCc).Get(), storages::redis::RequestFailedException);
}

UTEST_F(RedisClusterClientTest, Eval) {
    auto client = GetClient();

    /// [Sample eval usage]
    client->Set("the_key", "the_value", {}).Get();

    // ...
    const std::string kLuaScript{R"~(
    if redis.call("get",KEYS[1]) == ARGV[1] then
        redis.call("del",KEYS[1])
        return "del"
    else
        redis.call("rpush", "mismatched", KEYS[1])
        return "mismatched"
    end
)~"};

    auto val1 = client->Eval<std::string>(kLuaScript, {"the_key"}, {"mismatched_value"}, {}).Get();
    EXPECT_EQ(val1, "mismatched");

    auto val2 = client->Eval<std::string>(kLuaScript, {"the_key"}, {"the_value"}, {}).Get();
    EXPECT_EQ(val2, "del");
    /// [Sample eval usage]
}

UTEST_F(RedisClusterClientTest, EvalSha) {
    auto client = GetClient();

    /// [Sample evalsha usage]
    auto upload_scripts = [client]() {
        const std::string kLuaScript{R"~(
            if redis.call("get",KEYS[1]) == ARGV[1] then
                redis.call("del",KEYS[1])
                return "del"
            else
                redis.call("rpush", "mismatched", KEYS[1])
                return "mismatched"
            end
        )~"};
        const std::size_t shards_count = client->ShardsCount();
        std::string script_sha;
        for (std::size_t i = 0; i < shards_count; ++i) {
            script_sha = client->ScriptLoad(kLuaScript, i, {}).Get();
        }
        return script_sha;
    };
    const auto script_sha = upload_scripts();

    client->Set("the_key", "the_value", {}).Get();

    // ...

    auto val1 = client->EvalSha<std::string>(script_sha, {"the_key"}, {"mismatched_value"}, {}).Get();
    if (val1.IsNoScriptError()) {
        upload_scripts();

        // retry...
        val1 = client->EvalSha<std::string>(script_sha, {"the_key"}, {"mismatched_value"}, {}).Get();
    }
    EXPECT_EQ(val1.Get(), "mismatched");

    auto val2 = client->EvalSha<std::string>(script_sha, {"the_key"}, {"the_value"}, {}).Get();
    if (val2.IsNoScriptError()) {
        upload_scripts();

        // retry...
        val2 = client->EvalSha<std::string>(script_sha, {"the_key"}, {"the_value"}, {}).Get();
    }
    EXPECT_EQ(val2.Get(), "del");
    /// [Sample evalsha usage]

    // Make sure that it is fine to load the same script multiple times
    upload_scripts();
}

UTEST_F(RedisClusterClientTest, Subscribe) {
    auto client = GetClient();
    auto subscribe_client = GetSubscribeClient();

    const std::string kChannel1 = "channel01";
    const std::string kChannel2 = "channel02";
    const std::string kMsg1 = "test message1";
    const std::string kMsg2 = "test message2";

    engine::SingleConsumerEvent event1;
    engine::SingleConsumerEvent event2;
    size_t msg_counter = 0;
    const auto waiting_time = std::chrono::milliseconds(50);

    auto token1 = subscribe_client->Subscribe(kChannel1, [&](const std::string& channel, const std::string& message) {
        EXPECT_EQ(channel, kChannel1);
        EXPECT_EQ(message, kMsg1);
        ++msg_counter;
        event1.Send();
    });
    engine::SleepFor(waiting_time);

    client->Publish(kChannel1, kMsg1, kDefaultCc);
    ASSERT_TRUE(event1.WaitForEventFor(utest::kMaxTestWaitTime));
    EXPECT_EQ(msg_counter, 1);

    auto token2 = subscribe_client->Subscribe(kChannel2, [&](const std::string& channel, const std::string& message) {
        EXPECT_EQ(channel, kChannel2);
        EXPECT_EQ(message, kMsg2);
        ++msg_counter;
        event2.Send();
    });
    engine::SleepFor(waiting_time);

    client->Publish(kChannel2, kMsg2, kDefaultCc);
    ASSERT_TRUE(event2.WaitForEventFor(utest::kMaxTestWaitTime));
    EXPECT_EQ(msg_counter, 2);

    token1.Unsubscribe();
    client->Publish(kChannel1, kMsg1, kDefaultCc);
    engine::SleepFor(waiting_time);
    EXPECT_EQ(msg_counter, 2);

    client->Publish(kChannel2, kMsg2, kDefaultCc);
    ASSERT_TRUE(event2.WaitForEventFor(utest::kMaxTestWaitTime));
    EXPECT_EQ(msg_counter, 3);
}

// for manual testing of CLUSTER FAILOVER
UTEST_F(RedisClusterClientTest, LongWork) {
    const bool kIsManualTesting = false;
    const auto kTestTime = std::chrono::seconds(300);
    auto deadline = engine::Deadline::FromDuration(kTestTime);

    auto client = GetClient();

    const size_t kNumKeys = 10;
    const int add = 100;

    size_t num_write_errors = 0;
    size_t num_read_errors = 0;

    size_t iterations = 0;

    do {
        for (size_t i = 0; i < kNumKeys; ++i) {
            auto req = client->Set(MakeKey(i), std::to_string(add + i), kDefaultCc);
            try {
                req.Get();
            } catch (const storages::redis::RequestFailedException& ex) {
                ++num_write_errors;
                std::cerr << "Set failed with status " << ex.GetStatusString();
            }
        }

        for (size_t i = 0; i < kNumKeys; ++i) {
            auto req = client->Get(MakeKey(i), kDefaultCc);
            try {
                req.Get();
            } catch (const storages::redis::RequestFailedException& ex) {
                ++num_read_errors;
                std::cerr << "Get failed with status " << ex.GetStatusString();
            }
        }

        for (size_t i = 0; i < kNumKeys; ++i) {
            auto req = client->Del(MakeKey(i), kDefaultCc);
            try {
                req.Get();
            } catch (const storages::redis::RequestFailedException& ex) {
                ++num_write_errors;
                std::cerr << "Del failed with status " << ex.GetStatusString();
            }
        }

        ++iterations;
        engine::SleepFor(std::chrono::milliseconds(10));
    } while (!deadline.IsReached() && kIsManualTesting);

    EXPECT_EQ(num_write_errors, 0);
    EXPECT_EQ(num_read_errors, 0);
    EXPECT_GT(iterations, kIsManualTesting ? 100 : 0);
}

UTEST_F(RedisClusterClientTest, ClusterSlotsCalled) {
    auto client = GetClient();
    engine::SleepFor(std::chrono::seconds(10));
    ASSERT_GT(storages::redis::impl::ClusterSentinelImpl::GetClusterSlotsCalledCounter(), 2);
}

USERVER_NAMESPACE_END
