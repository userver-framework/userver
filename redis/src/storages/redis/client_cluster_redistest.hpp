#pragma once

#include <storages/redis/util_redistest.hpp>

USERVER_NAMESPACE_BEGIN

class RedisClusterClientTest : public BaseRedisClusterClientTest {
public:
    void SetUp() override {
        BaseRedisClusterClientTest::SetUp();

        for (size_t shard = 0; shard < GetSentinel()->ShardsCount(); ++shard) {
            GetSentinel()->MakeRequest({"flushdb"}, shard, true).Get();
        }
    }
};

USERVER_NAMESPACE_END
