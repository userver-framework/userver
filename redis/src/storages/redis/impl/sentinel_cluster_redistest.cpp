#include <utest/utest.hpp>

#include <memory>

#include <engine/sleep.hpp>
#include <formats/json/serialize.hpp>
#include <storages/redis/redis_secdist.hpp>

#include <storages/redis/impl/keyshard_impl.hpp>
#include <storages/redis/impl/reply.hpp>
#include <storages/redis/impl/sentinel.hpp>
#include <storages/redis/impl/subscribe_sentinel.hpp>
#include <storages/redis/impl/transaction.hpp>

namespace {

const std::string kRedisSettingsJson = R"({
  "redis_settings": {
    "cluster-test": {
      "password": "",
      "sentinels": [
        {"host": "localhost", "port": 7100},
        {"host": "localhost", "port": 7101},
        {"host": "localhost", "port": 7102}
      ],
      "shards": [
        {"name": "master0"},
        {"name": "master1"},
        {"name": "master2"}
      ]
    }
  }
})";

const storages::secdist::RedisMapSettings kRedisSettings{
    formats::json::FromString(kRedisSettingsJson)};

class TestSentinel {
 public:
  TestSentinel()
      : thread_pools_(std::make_shared<redis::ThreadPools>(
            redis::kDefaultSentinelThreadPoolSize,
            redis::kDefaultRedisThreadPoolSize)),
        sentinel_(redis::Sentinel::CreateSentinel(
            thread_pools_, kRedisSettings.GetSettings("cluster-test"),
            "cluster-test", "cluster-test-client_name",
            redis::KeyShardFactory{::redis::kRedisCluster}, {})),
        subscribe_sentinel_(redis::SubscribeSentinel::Create(
            thread_pools_, kRedisSettings.GetSettings("cluster-test"),
            "cluster-test", "cluster-test-client_name", true, {})) {
    sentinel_->WaitConnectedDebug();
  }

  std::shared_ptr<redis::Sentinel> GetSentinel() const { return sentinel_; }
  std::shared_ptr<redis::SubscribeSentinel> GetSubscribeSentinel() const {
    return subscribe_sentinel_;
  }

 private:
  std::shared_ptr<redis::ThreadPools> thread_pools_;
  std::shared_ptr<redis::Sentinel> sentinel_;
  std::shared_ptr<redis::SubscribeSentinel> subscribe_sentinel_;
};

const std::string kKeyNamePrefix = "test_key_";

std::string MakeKey(size_t idx) { return kKeyNamePrefix + std::to_string(idx); }

std::string MakeKey2(size_t idx, int add) {
  return "{" + MakeKey(idx) + "}not_hashed_suffix_" + std::to_string(add - idx);
}

redis::CommandControl kDefaultCc(std::chrono::milliseconds(300),
                                 std::chrono::milliseconds(300), 1);

}  // namespace

// Tests are disabled because no local redis cluster is running by default.
// See https://st.yandex-team.ru/TAXICOMMON-2397#5eb61160968f92029663522c for
// details.
TEST(DISABLED_SentinelCluster, SetGet) {
  RunInCoro([] {
    TestSentinel test_sentinel;
    auto sentinel = test_sentinel.GetSentinel();

    const size_t kNumKeys = 10;
    const int add = 100;

    for (size_t i = 0; i < kNumKeys; ++i) {
      auto req = sentinel->Set(kKeyNamePrefix + std::to_string(i),
                               std::to_string(add + i));
      auto reply = req.Get();
      EXPECT_TRUE(reply->IsOk());
    }

    for (size_t i = 0; i < kNumKeys; ++i) {
      auto req = sentinel->Get(kKeyNamePrefix + std::to_string(i), kDefaultCc);
      auto reply = req.Get();
      ASSERT_TRUE(reply->IsOk());
      ASSERT_TRUE(reply->data.IsString());
      EXPECT_EQ(reply->data.GetString(), std::to_string(add + i));
    }

    for (size_t i = 0; i < kNumKeys; ++i) {
      auto req = sentinel->Del(kKeyNamePrefix + std::to_string(i));
      auto reply = req.Get();
      EXPECT_TRUE(reply->IsOk());
    }
  });
}

TEST(DISABLED_SentinelCluster, Mget) {
  RunInCoro([] {
    TestSentinel test_sentinel;
    auto sentinel = test_sentinel.GetSentinel();

    const size_t kNumKeys = 10;
    const int add = 100;

    for (size_t i = 0; i < kNumKeys; ++i) {
      auto req = sentinel->Set(MakeKey(i), std::to_string(add + i));
      auto reply = req.Get();
      EXPECT_TRUE(reply->IsOk());
    }

    for (size_t i = 0; i < kNumKeys; ++i) {
      auto req = sentinel->Set(MakeKey2(i, add), std::to_string(add * 2 + i));
      auto reply = req.Get();
      EXPECT_TRUE(reply->IsOk());
    }

    for (size_t i = 0; i < kNumKeys; ++i) {
      auto req = sentinel->Mget({MakeKey(i), MakeKey2(i, add)}, kDefaultCc);
      auto reply = req.Get();
      ASSERT_TRUE(reply->IsOk());
      ASSERT_TRUE(reply->data.IsArray())
          << "type=" << reply->data.GetTypeString()
          << " msg=" << reply->data.ToDebugString();
      ASSERT_TRUE(reply->data.GetArray().size() == 2);

      ASSERT_TRUE(reply->data.GetArray()[0].IsString());
      EXPECT_EQ(reply->data.GetArray()[0].GetString(), std::to_string(add + i));
      ASSERT_TRUE(reply->data.GetArray()[1].IsString());
      EXPECT_EQ(reply->data.GetArray()[1].GetString(),
                std::to_string(add * 2 + i));
    }

    for (size_t i = 0; i < kNumKeys; ++i) {
      auto req = sentinel->Del(MakeKey(i));
      auto reply = req.Get();
      EXPECT_TRUE(reply->IsOk());
    }

    for (size_t i = 0; i < kNumKeys; ++i) {
      auto req = sentinel->Del(MakeKey2(i, add));
      auto reply = req.Get();
      EXPECT_TRUE(reply->IsOk());
    }
  });
}

TEST(DISABLED_SentinelCluster, MgetCrossSlot) {
  RunInCoro([] {
    TestSentinel test_sentinel;
    auto sentinel = test_sentinel.GetSentinel();

    const int add = 100;

    size_t idx[2] = {0, 1};
    auto shard = sentinel->ShardByKey(MakeKey(idx[0]));
    while (sentinel->ShardByKey(MakeKey(idx[1])) != shard) ++idx[1];

    for (size_t i = 0; i < 2; ++i) {
      auto req = sentinel->Set(MakeKey(idx[i]), std::to_string(add + i));
      auto reply = req.Get();
      EXPECT_TRUE(reply->IsOk());
    }

    {
      auto req = sentinel->Mget({MakeKey(idx[0]), MakeKey(idx[1])}, kDefaultCc);
      auto reply = req.Get();
      ASSERT_TRUE(reply->IsOk());
      ASSERT_TRUE(reply->data.IsError())
          << "type=" << reply->data.GetTypeString()
          << " msg=" << reply->data.ToDebugString();
    }

    for (size_t i = 0; i < 2; ++i) {
      auto req = sentinel->Del(MakeKey(idx[i]));
      auto reply = req.Get();
      EXPECT_TRUE(reply->IsOk());
    }
  });
}

TEST(DISABLED_SentinelCluster, Transaction) {
  RunInCoro([] {
    TestSentinel test_sentinel;
    auto sentinel = test_sentinel.GetSentinel();

    const int add = 100;

    redis::Transaction transaction(sentinel);

    transaction.Set(MakeKey(0), std::to_string(add));
    transaction.Get(MakeKey(0));
    transaction.Set(MakeKey2(0, add), std::to_string(add + 1));
    transaction.Get(MakeKey2(0, add));

    auto req = transaction.Exec(kDefaultCc);
    auto reply = req.Get();
    ASSERT_TRUE(reply->IsOk());
    ASSERT_TRUE(reply->data.IsArray())
        << reply->data.GetTypeString() << " " << reply->data.ToDebugString();

    const auto& array = reply->data.GetArray();
    for (size_t i = 0; i < 2; ++i) {
      EXPECT_TRUE(array[i * 2].IsStatus());
      ASSERT_TRUE(array[i * 2 + 1].IsString());
      EXPECT_EQ(array[i * 2 + 1].GetString(), std::to_string(add + i));
    }

    {
      auto req = sentinel->Del(MakeKey(0));
      auto reply = req.Get();
      EXPECT_TRUE(reply->IsOk());
    }
    {
      auto req = sentinel->Del(MakeKey2(0, add));
      auto reply = req.Get();
      EXPECT_TRUE(reply->IsOk());
    }
  });
}

TEST(DISABLED_SentinelCluster, TransactionCrossSlot) {
  RunInCoro([] {
    TestSentinel test_sentinel;
    auto sentinel = test_sentinel.GetSentinel();

    const int add = 100;

    size_t idx[2] = {0, 1};
    auto shard = sentinel->ShardByKey(MakeKey(idx[0]));
    while (sentinel->ShardByKey(MakeKey(idx[1])) != shard) ++idx[1];

    redis::Transaction transaction(sentinel);

    for (size_t i = 0; i < 2; ++i) {
      transaction.Set(MakeKey(idx[i]), std::to_string(add + i));
      transaction.Get(MakeKey(idx[i]));
    }
    auto req = transaction.Exec(kDefaultCc);
    auto reply = req.Get();
    ASSERT_TRUE(reply->IsOk());
    EXPECT_TRUE(reply->data.IsError());
  });
}

TEST(DISABLED_SentinelCluster, TransactionDistinctShards) {
  RunInCoro([] {
    TestSentinel test_sentinel;
    auto sentinel = test_sentinel.GetSentinel();

    const size_t kNumKeys = 10;
    const int add = 100;

    redis::Transaction transaction(sentinel,
                                   ::redis::Transaction::CheckShards::kNo);

    for (size_t i = 0; i < kNumKeys; ++i) {
      transaction.Set(MakeKey(i), std::to_string(add + i));
      transaction.Get(MakeKey(i));
    }
    auto req = transaction.Exec(kDefaultCc);
    auto reply = req.Get();
    ASSERT_TRUE(reply->IsOk());
    EXPECT_TRUE(reply->data.IsError());
  });
}

TEST(DISABLED_SentinelCluster, Subscribe) {
  RunInCoro([] {
    TestSentinel test_sentinel;
    auto sentinel = test_sentinel.GetSentinel();
    auto subscribe_sentinel = test_sentinel.GetSubscribeSentinel();
    const std::string kChannel1 = "channel01";
    const std::string kChannel2 = "channel02";
    const std::string kMsg1 = "test message1";
    const std::string kMsg2 = "test message2";
    size_t msg_counter = 0;
    const auto waiting_time = std::chrono::milliseconds(50);

    auto token1 = subscribe_sentinel->Subscribe(
        kChannel1, [&](const std::string& channel, const std::string& message) {
          EXPECT_EQ(channel, kChannel1);
          EXPECT_EQ(message, kMsg1);
          ++msg_counter;
        });
    engine::SleepFor(waiting_time);

    sentinel->Publish(kChannel1, kMsg1);
    engine::SleepFor(waiting_time);

    EXPECT_EQ(msg_counter, 1);

    auto token2 = subscribe_sentinel->Subscribe(
        kChannel2, [&](const std::string& channel, const std::string& message) {
          EXPECT_EQ(channel, kChannel2);
          EXPECT_EQ(message, kMsg2);
          ++msg_counter;
        });
    engine::SleepFor(waiting_time);

    sentinel->Publish(kChannel2, kMsg2);
    engine::SleepFor(waiting_time);

    EXPECT_EQ(msg_counter, 2);

    token1.Unsubscribe();
    sentinel->Publish(kChannel1, kMsg1);
    engine::SleepFor(waiting_time);

    EXPECT_EQ(msg_counter, 2);

    sentinel->Publish(kChannel2, kMsg2);
    engine::SleepFor(waiting_time);

    EXPECT_EQ(msg_counter, 3);
  });
}
