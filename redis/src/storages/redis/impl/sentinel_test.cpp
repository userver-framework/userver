#include <storages/redis/impl/keyshard_impl.hpp>
#include <userver/storages/redis/impl/sentinel.hpp>

#include <gtest/gtest.h>

using namespace redis;

TEST(Sentinel, DISABLED_PublishPerformance) {
  auto thread_pools = std::make_shared<redis::ThreadPools>(
      redis::kDefaultSentinelThreadPoolSize,
      redis::kDefaultRedisThreadPoolSize);
  Sentinel s(thread_pools, {""}, {}, "test_shard_group", "test",
             redis::Password(""),
             [](size_t, const std::string&, bool, bool) {});
  auto start = std::chrono::high_resolution_clock::now();
  for (auto i = 0; i < 40000; ++i) {
    s.Publish("channel", "message");
  }
  auto end = std::chrono::high_resolution_clock::now();

  ASSERT_LT(end - start, std::chrono::seconds(1));
}

TEST(Sentinel, CreateTmpKey) {
  const KeyShardCrc32 key_shard(0xffffffff);
  for (const char* const key : {"hello:world", "abc", "duke:nukem{must:die}"}) {
    const std::string& tmpkey = Sentinel::CreateTmpKey(key);
    EXPECT_STRNE(key, tmpkey.c_str()) << key << " vs " << tmpkey;
    EXPECT_EQ(key_shard.ShardByKey(key), key_shard.ShardByKey(tmpkey))
        << key << " vs " << tmpkey;
  }
}
