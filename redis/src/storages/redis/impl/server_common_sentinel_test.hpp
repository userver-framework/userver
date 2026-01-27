#pragma once

#include <storages/redis/impl/thread_pools.hpp>
#include <userver/dynamic_config/test_helpers.hpp>

#include <storages/redis/impl/keyshard_impl.hpp>
#include <storages/redis/impl/mock_server_test.hpp>
#include <storages/redis/impl/secdist_redis.hpp>
#include <storages/redis/impl/sentinel.hpp>

USERVER_NAMESPACE_BEGIN

// 100ms should be enough, but valgrind is too slow
inline constexpr auto kSmallPeriod = std::chrono::milliseconds(500);

const std::string kLocalhost = "127.0.0.1";

class ClusterTest {
public:
    explicit ClusterTest(
        size_t master_count,
        int magic_value_add_master = 0,
        int magic_value_add_slave = 0,
        size_t redis_thread_count = 1,
        storages::redis::impl::KeyShardFactory key_shard = {storages::redis::impl::kRedisCluster}
    );

    using MockRedisServerArray = std::vector<std::unique_ptr<MockRedisServer>>;

    storages::redis::impl::Sentinel& SentinelClient() const { return *sentinel_client_; }

    MockRedisServerArray& Masters() { return masters_; }
    MockRedisServerArray& Slaves() { return slaves_; }

    MockRedisServer& Master(size_t idx = 0) { return *masters_.at(idx); }
    MockRedisServer& Slave(size_t idx = 0) { return *slaves_.at(idx); }

    const std::string& RedisName() const { return redis_name_; }

private:
    void InitClusterSentinelServers();
    void CreateSentinelClient(storages::redis::impl::KeyShardFactory key_shard);

    const std::string redis_name_{"redis_cluster_name"};
    MockRedisServerArray masters_;
    MockRedisServerArray slaves_;

    std::shared_ptr<storages::redis::impl::ThreadPools> thread_pools_;
    std::shared_ptr<storages::redis::impl::Sentinel> sentinel_client_;
};

class SentinelTest {
public:
    SentinelTest(
        size_t sentinel_count,
        size_t master_count,
        size_t slave_count,
        int magic_value_add_master = 0,
        int magic_value_add_slave = 0,
        size_t redis_thread_count = 1,
        storages::redis::impl::KeyShardFactory key_shard = {""}
    );

    using MockRedisServerArray = std::vector<std::unique_ptr<MockRedisServer>>;

    storages::redis::impl::Sentinel& SentinelClient() const { return *sentinel_client_; }

    MockRedisServerArray& Masters() { return masters_; }
    MockRedisServerArray& Slaves() { return slaves_; }
    MockRedisServerArray& Sentinels() { return sentinels_; }

    MockRedisServer& Master(size_t idx = 0) { return *masters_.at(idx); }
    MockRedisServer& Slave(size_t idx = 0) { return *slaves_.at(idx); }
    MockRedisServer& Sentinel(size_t idx = 0) { return *sentinels_.at(idx); }

    const std::string& RedisName() const { return redis_name_; }

private:
    void InitSentinelServers();
    void CreateSentinelClient(storages::redis::impl::KeyShardFactory key_shard);

    const std::string redis_name_{"redis_name"};
    MockRedisServerArray masters_;
    MockRedisServerArray slaves_;
    MockRedisServerArray sentinels_;

    std::shared_ptr<storages::redis::impl::ThreadPools> thread_pools_;
    std::shared_ptr<storages::redis::impl::Sentinel> sentinel_client_;
};

class SentinelShardTest {
public:
    SentinelShardTest(
        size_t sentinel_count,
        size_t shard_count,
        int magic_value_add_master = 0,
        int magic_value_add_slave = 0,
        size_t redis_thread_count = 1,
        storages::redis::impl::KeyShardFactory key_shard = {""}
    );

    using MockRedisServerArray = std::vector<std::unique_ptr<MockRedisServer>>;

    storages::redis::impl::Sentinel& SentinelClient() const { return *sentinel_client_; }

    MockRedisServerArray& Masters() { return masters_; }
    MockRedisServerArray& Slaves() { return slaves_; }
    MockRedisServerArray& Sentinels() { return sentinels_; }

    MockRedisServer& Master(size_t idx = 0) { return *masters_.at(idx); }
    MockRedisServer& Slave(size_t idx = 0) { return *slaves_.at(idx); }
    MockRedisServer& Sentinel(size_t idx = 0) { return *sentinels_.at(idx); }

    const std::string& RedisName(size_t idx) const { return redis_names_.at(idx); }

private:
    static std::vector<std::string> InitRedisNames(size_t shard_count);
    void InitSentinelServers(size_t shard_count);
    void CreateSentinelClient(storages::redis::impl::KeyShardFactory key_shard);

    const std::vector<std::string> redis_names_;
    MockRedisServerArray masters_;
    MockRedisServerArray slaves_;
    MockRedisServerArray sentinels_;

    std::shared_ptr<storages::redis::impl::ThreadPools> thread_pools_;
    std::shared_ptr<storages::redis::impl::Sentinel> sentinel_client_;
};

USERVER_NAMESPACE_END
