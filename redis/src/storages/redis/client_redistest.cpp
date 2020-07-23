#include <utest/utest.hpp>

#include <memory>
#include <string>

#include <formats/json/serialize.hpp>
#include <storages/redis/client_impl.hpp>
#include <storages/redis/impl/sentinel.hpp>
#include <storages/redis/impl/thread_pools.hpp>
#include <storages/redis/redis_secdist.hpp>

namespace {

const std::string kRedisSettingsJson = R"({
  "redis_settings": {
    "taxi-test": {
        "command_control": {
            "max_retries": 1,
            "timeout_all_ms": 60000,
            "timeout_single_ms": 60000
        },
        "password": "",
        "sentinels": [{"host": "localhost", "port": 26379}],
        "shards": [{"name": "test_master0"}]
    }
  }
})";

std::shared_ptr<storages::redis::Client> GetClient() {
  static const storages::secdist::RedisMapSettings kRedisSettings(
      formats::json::FromString(kRedisSettingsJson));
  const auto& settings = kRedisSettings.GetSettings("taxi-test");
  auto thread_pools = std::make_shared<redis::ThreadPools>(
      redis::kDefaultSentinelThreadPoolSize,
      redis::kDefaultRedisThreadPoolSize);
  auto sentinel =
      redis::Sentinel::CreateSentinel(std::move(thread_pools), settings, "none",
                                      "pub", redis::KeyShardFactory{""}, {});
  sentinel->WaitConnectedDebug();

  return std::make_shared<storages::redis::ClientImpl>(std::move(sentinel));
}

}  // namespace

TEST(RedisClient, Lrem) {
  RunInCoro([] {
    auto client = GetClient();
    EXPECT_EQ(client->Rpush("testlist", "a", {}).Get(), 1);
    EXPECT_EQ(client->Rpush("testlist", "b", {}).Get(), 2);
    EXPECT_EQ(client->Rpush("testlist", "a", {}).Get(), 3);
    EXPECT_EQ(client->Rpush("testlist", "b", {}).Get(), 4);
    EXPECT_EQ(client->Rpush("testlist", "b", {}).Get(), 5);
    EXPECT_EQ(client->Llen("testlist", {}).Get(), 5);

    EXPECT_EQ(client->Lrem("testlist", 1, "b", {}).Get(), 1);
    EXPECT_EQ(client->Llen("testlist", {}).Get(), 4);
    EXPECT_EQ(client->Lrem("testlist", -2, "b", {}).Get(), 2);
    EXPECT_EQ(client->Llen("testlist", {}).Get(), 2);
    EXPECT_EQ(client->Lrem("testlist", 0, "c", {}).Get(), 0);
    EXPECT_EQ(client->Llen("testlist", {}).Get(), 2);
    EXPECT_EQ(client->Lrem("testlist", 0, "a", {}).Get(), 2);
    EXPECT_EQ(client->Llen("testlist", {}).Get(), 0);
  });
}

TEST(RedisClient, Lpushx) {
  RunInCoro([] {
    auto client = GetClient();
    // Missing array - does nothing
    EXPECT_EQ(client->Lpushx("pushx_testlist", "a", {}).Get(), 0);
    EXPECT_EQ(client->Rpushx("pushx_testlist", "a", {}).Get(), 0);
    // // Init list
    EXPECT_EQ(client->Lpush("pushx_testlist", "a", {}).Get(), 1);
    // // List exists - appends values
    EXPECT_EQ(client->Lpushx("pushx_testlist", "a", {}).Get(), 2);
    EXPECT_EQ(client->Rpushx("pushx_testlist", "a", {}).Get(), 3);
  });
}
