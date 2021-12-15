#include <storages/redis/impl/keyshard_impl.hpp>
#include <userver/storages/redis/impl/sentinel.hpp>

#include <gtest/gtest.h>

using namespace USERVER_NAMESPACE::redis;

TEST(Sentinel, CreateTmpKey) {
  const KeyShardCrc32 key_shard(0xffffffff);
  for (const char* const key : {"hello:world", "abc", "duke:nukem{must:die}"}) {
    const std::string& tmpkey = Sentinel::CreateTmpKey(key);
    EXPECT_STRNE(key, tmpkey.c_str()) << key << " vs " << tmpkey;
    EXPECT_EQ(key_shard.ShardByKey(key), key_shard.ShardByKey(tmpkey))
        << key << " vs " << tmpkey;
  }
}
