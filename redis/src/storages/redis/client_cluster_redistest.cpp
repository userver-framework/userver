#include <userver/utest/utest.hpp>

#include <memory>

#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/sleep.hpp>

#include <storages/redis/client_cluster_redistest.hpp>
#include <storages/redis/client_impl.hpp>
#include <storages/redis/dynamic_config.hpp>
#include <storages/redis/impl/cluster_sentinel_impl.hpp>
#include <storages/redis/impl/keyshard_impl.hpp>
#include <storages/redis/impl/sentinel.hpp>
#include <storages/redis/impl/subscribe_sentinel.hpp>
#include <storages/redis/subscribe_client_impl.hpp>
#include <storages/redis/util_redistest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const std::string kKeyNamePrefix = "test_key_";

std::string MakeKey(size_t idx) { return kKeyNamePrefix + std::to_string(idx); }

std::string MakeKey2(size_t idx, int add) {
  return "{" + MakeKey(idx) + "}not_hashed_suffix_" + std::to_string(add - idx);
}

redis::CommandControl kDefaultCc(std::chrono::milliseconds(300),
                                 std::chrono::milliseconds(300), 1);

}  // namespace

// Tests are disabled because no local redis cluster is running by default.
// See https://st.yandex-team.ru/TAXICOMMON-2440#5ecf09f0ffc9d004c04c43b1 for
// details.
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
    auto req =
        client->Set(MakeKey2(i, add), std::to_string(add * 2 + i), kDefaultCc);
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

  for (unsigned long i : idx) {
    auto req = client->Set(MakeKey(i), std::to_string(add + i), kDefaultCc);
    UASSERT_NO_THROW(req.Get());
  }

  {
    auto req = client->Mget({MakeKey(idx[0]), MakeKey(idx[1])}, kDefaultCc);
    UASSERT_THROW(req.Get(), redis::ParseReplyException);
  }

  for (unsigned long i : idx) {
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
  UASSERT_THROW(transaction->Exec(kDefaultCc).Get(),
                redis::ParseReplyException);
}

UTEST_F(RedisClusterClientTest, TransactionDistinctShards) {
  auto client = GetClient();
  auto transaction =
      client->Multi(storages::redis::Transaction::CheckShards::kNo);

  const size_t kNumKeys = 10;
  const int add = 100;

  for (size_t i = 0; i < kNumKeys; ++i) {
    auto set = transaction->Set(MakeKey(i), std::to_string(add + i));
    auto get = transaction->Get(MakeKey(i));
  }
  UASSERT_THROW(transaction->Exec(kDefaultCc).Get(),
                redis::ParseReplyException);
}

UTEST_F(RedisClusterClientTest, Subscribe) {
  auto client = GetClient();
  auto subscribe_client = GetSubscribeClient();

  const std::string kChannel1 = "channel01";
  const std::string kChannel2 = "channel02";
  const std::string kMsg1 = "test message1";
  const std::string kMsg2 = "test message2";
  size_t msg_counter = 0;
  const auto waiting_time = std::chrono::milliseconds(50);

  auto token1 = subscribe_client->Subscribe(
      kChannel1, [&](const std::string& channel, const std::string& message) {
        EXPECT_EQ(channel, kChannel1);
        EXPECT_EQ(message, kMsg1);
        ++msg_counter;
      });
  engine::SleepFor(waiting_time);

  client->Publish(kChannel1, kMsg1, kDefaultCc);
  engine::SleepFor(waiting_time);

  EXPECT_EQ(msg_counter, 1);

  auto token2 = subscribe_client->Subscribe(
      kChannel2, [&](const std::string& channel, const std::string& message) {
        EXPECT_EQ(channel, kChannel2);
        EXPECT_EQ(message, kMsg2);
        ++msg_counter;
      });
  engine::SleepFor(waiting_time);

  client->Publish(kChannel2, kMsg2, kDefaultCc);
  engine::SleepFor(waiting_time);

  EXPECT_EQ(msg_counter, 2);

  token1.Unsubscribe();
  client->Publish(kChannel1, kMsg1, kDefaultCc);
  engine::SleepFor(waiting_time);

  EXPECT_EQ(msg_counter, 2);

  client->Publish(kChannel2, kMsg2, kDefaultCc);
  engine::SleepFor(waiting_time);

  EXPECT_EQ(msg_counter, 3);
}

// for manual testing of CLUSTER FAILOVER
UTEST_F(RedisClusterClientTest, DISABLED_LongWork) {
  const auto kTestTime = std::chrono::seconds(30);
  auto deadline = engine::Deadline::FromDuration(kTestTime);

  auto client = GetClient();

  const size_t kNumKeys = 10;
  const int add = 100;

  size_t num_write_errors = 0;
  size_t num_read_errors = 0;

  size_t iterations = 0;

  while (!deadline.IsReached()) {
    for (size_t i = 0; i < kNumKeys; ++i) {
      auto req = client->Set(MakeKey(i), std::to_string(add + i), kDefaultCc);
      try {
        req.Get();
      } catch (const redis::RequestFailedException& ex) {
        ++num_write_errors;
        std::cerr << "Set failed with status " << ex.GetStatusString();
      }
    }

    for (size_t i = 0; i < kNumKeys; ++i) {
      auto req = client->Get(MakeKey(i), kDefaultCc);
      try {
        req.Get();
      } catch (const redis::RequestFailedException& ex) {
        ++num_read_errors;
        std::cerr << "Get failed with status " << ex.GetStatusString();
      }
    }

    for (size_t i = 0; i < kNumKeys; ++i) {
      auto req = client->Del(MakeKey(i), kDefaultCc);
      try {
        req.Get();
      } catch (const redis::RequestFailedException& ex) {
        ++num_write_errors;
        std::cerr << "Del failed with status " << ex.GetStatusString();
      }
    }

    ++iterations;
    engine::SleepFor(std::chrono::milliseconds(10));
  }

  EXPECT_EQ(num_write_errors, 0);
  EXPECT_EQ(num_read_errors, 0);
  EXPECT_GT(iterations, 100);
}

UTEST_F(RedisClusterClientTest, ClusterSlotsCalled) {
  auto client = GetClient();
  engine::SleepFor(std::chrono::seconds(10));
  ASSERT_GT(redis::ClusterSentinelImpl::GetClusterSlotsCalledCounter(), 2);
}

USERVER_NAMESPACE_END
