#include <userver/utest/using_namespace_userver.hpp>

/// [Unit test]
#include <userver/storages/redis/utest/redis_fixture.hpp>

using RedisTest = storages::redis::utest::RedisTest;

UTEST_F(RedisTest, Sample) {
  auto client = GetClient();

  client->Rpush("sample_list", "a", {}).Get();
  client->Rpush("sample_list", "b", {}).Get();

  const auto length = client->Llen("sample_list", {}).Get();
  EXPECT_EQ(length, 2);
}
/// [Unit test]
