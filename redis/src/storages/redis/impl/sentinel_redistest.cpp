#include <utest/utest.hpp>

#include <memory>

#include <engine/sleep.hpp>
#include <formats/json/serialize.hpp>
#include <storages/redis/redis_secdist.hpp>

#include <storages/redis/impl/reply.hpp>
#include <storages/redis/impl/sentinel.hpp>

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

const storages::secdist::RedisMapSettings kRedisSettings{
    formats::json::FromString(kRedisSettingsJson)};

}  // namespace

UTEST(Sentinel, ReplyServerId) {
  /* TODO: hack! sentinel is too slow to learn new replicaset members :-( */
  engine::SleepFor(std::chrono::milliseconds(11000));

  auto settings = kRedisSettings.GetSettings("taxi-test");
  auto thread_pools = std::make_shared<redis::ThreadPools>(
      redis::kDefaultSentinelThreadPoolSize,
      redis::kDefaultRedisThreadPoolSize);
  auto sentinel = redis::Sentinel::CreateSentinel(
      thread_pools, settings, "none", "pub", redis::KeyShardFactory{""}, {});
  sentinel->WaitConnectedDebug();

  auto req = sentinel->Keys("*", 0);
  auto reply = req.Get();
  auto first_id = reply->server_id;
  ASSERT_FALSE(first_id.IsAny());

  // We want at least 1 id != first_id
  const auto max_i = 10;
  bool has_any_distinct_id = false;
  for (int i = 0; i < max_i; i++) {
    auto req = sentinel->Keys("*", 0);
    auto reply = req.Get();
    auto id = reply->server_id;
    if (!(id == first_id)) has_any_distinct_id = true;

    EXPECT_TRUE(reply->IsOk());
    EXPECT_FALSE(id.IsAny());
  }
  EXPECT_TRUE(has_any_distinct_id);
}

UTEST(Sentinel, ForceServerId) {
  auto settings = kRedisSettings.GetSettings("taxi-test");
  auto thread_pools = std::make_shared<redis::ThreadPools>(
      redis::kDefaultSentinelThreadPoolSize,
      redis::kDefaultRedisThreadPoolSize);
  auto sentinel = redis::Sentinel::CreateSentinel(
      thread_pools, settings, "none", "pub", redis::KeyShardFactory{""}, {});
  sentinel->WaitConnectedDebug();

  auto req = sentinel->Keys("*", 0);
  auto reply = req.Get();
  auto first_id = reply->server_id;
  EXPECT_FALSE(first_id.IsAny());

  const auto max_i = 10;
  for (int i = 0; i < max_i; i++) {
    redis::CommandControl cc;
    cc.force_server_id = first_id;

    auto req = sentinel->Keys("*", 0, cc);
    auto reply = req.Get();
    auto id = reply->server_id;

    EXPECT_TRUE(reply->IsOk());
    EXPECT_FALSE(id.IsAny());
    EXPECT_EQ(first_id, id);
  }
}

UTEST(Sentinel, ForceNonExistingServerId) {
  auto settings = kRedisSettings.GetSettings("taxi-test");
  auto thread_pools = std::make_shared<redis::ThreadPools>(
      redis::kDefaultSentinelThreadPoolSize,
      redis::kDefaultRedisThreadPoolSize);
  auto sentinel = redis::Sentinel::CreateSentinel(
      thread_pools, settings, "none", "pub", redis::KeyShardFactory{""}, {});
  sentinel->WaitConnectedDebug();

  // w/o force_server_id
  auto req1 = sentinel->Keys("*", 0);
  auto reply1 = req1.Get();
  EXPECT_TRUE(reply1->IsOk());

  // w force_server_id
  redis::CommandControl cc;
  cc.force_server_id = redis::ServerId::Invalid();
  auto req2 = sentinel->Keys("*", 0, cc);
  auto reply2 = req2.Get();

  EXPECT_FALSE(reply2->IsOk());
}
