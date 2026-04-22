#pragma once

#include <storages/redis/util_redistest.hpp>

USERVER_NAMESPACE_BEGIN

class RedisClientTest : public BaseRedisClientTest {
public:
    void SetUp() override {
        BaseRedisClientTest::SetUp();

        for (size_t shard = 0; shard < GetSentinel()->ShardsCount(); shard++) {
            GetSentinel()->MakeRequest({"flushdb"}, shard, true).Get();
        }
    }
};

USERVER_NAMESPACE_END
