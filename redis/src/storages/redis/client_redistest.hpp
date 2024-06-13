#pragma once

#include <storages/redis/util_redistest.hpp>

USERVER_NAMESPACE_BEGIN

class RedisClientTest : public BaseRedisClientTest {
 public:
  void SetUp() override {
    BaseRedisClientTest::SetUp();

    GetSentinel()->MakeRequest({"flushdb"}, "none", true).Get();
  }
};

USERVER_NAMESPACE_END
