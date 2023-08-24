#include <storages/redis/impl/keyshard_impl.hpp>
#include <storages/redis/impl/sentinel.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(Sentinel, CreateTmpKey) {
  const redis::KeyShardCrc32 key_shard(0xffffffff);
  for (const char* const key : {"hello:world", "abc", "duke:nukem{must:die}"}) {
    const std::string& tmpkey = redis::Sentinel::CreateTmpKey(key);
    EXPECT_STRNE(key, tmpkey.c_str()) << key << " vs " << tmpkey;
    EXPECT_EQ(key_shard.ShardByKey(key), key_shard.ShardByKey(tmpkey))
        << key << " vs " << tmpkey;
  }
}

USERVER_NAMESPACE_END
