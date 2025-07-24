#include "server_common_sentinel_test.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

using MockRedisServerArray = std::vector<std::unique_ptr<MockRedisServer>>;

MockRedisServerArray
InitServerArray(size_t size, const std::string& description, std::optional<int> magic_value_add = {}) {
    MockRedisServerArray servers;
    for (size_t i = 0; i < size; i++) {
        servers.emplace_back(std::make_unique<MockRedisServer>(description + '-' + std::to_string(i)));
        auto& server = *servers.back();
        server.RegisterPingHandler();
        if (magic_value_add) server.RegisterHandlerWithConstReply("GET", *magic_value_add + i);
        LOG_DEBUG() << description << '[' << i << "].port=" << server.GetPort();
    }
    return servers;
}

}  // namespace

ClusterTest::ClusterTest(
    size_t master_count,
    int magic_value_add_master,
    int magic_value_add_slave,
    size_t redis_thread_count,
    storages::redis::impl::KeyShardFactory key_shard
)
    : masters_(InitServerArray(master_count, "masters", magic_value_add_master)),
      slaves_(InitServerArray(master_count, "slaves", magic_value_add_slave)),
      thread_pools_(std::make_shared<storages::redis::impl::ThreadPools>(1, redis_thread_count)) {
    UASSERT(key_shard.IsClusterStrategy());
    InitClusterSentinelServers();
    CreateSentinelClient(std::move(key_shard));
}

void ClusterTest::InitClusterSentinelServers() {
    std::vector<MockRedisServer::MasterInfo> master_infos;
    for (const auto& master : masters_) master_infos.emplace_back(redis_name_, kLocalhost, master->GetPort());

    std::vector<MockRedisServer::SlaveInfo> slave_infos;
    for (const auto& slave : slaves_) slave_infos.emplace_back(redis_name_, kLocalhost, slave->GetPort());

    for (const auto& slave : slaves_) {
        slave->RegisterClusterNodes(master_infos, slave_infos);
        slave->RegisterStatusReplyHandler("READONLY", "OK");
    }

    for (const auto& master : masters_) {
        master->RegisterClusterNodes(master_infos, slave_infos);
        // We send "READONLY" for connection in advance to failover, so that when master becomes a replica we do not
        // need to recreate the connection or to send "READONLY".
        master->RegisterStatusReplyHandler("READONLY", "OK");
    }
}

void ClusterTest::CreateSentinelClient(storages::redis::impl::KeyShardFactory key_shard) {
    secdist::RedisSettings settings;
    UASSERT(key_shard.IsClusterStrategy());
    for (const auto& master : masters_) {
        settings.sentinels.emplace_back(kLocalhost, master->GetPort());
        settings.shards.emplace_back(redis_name_ + std::to_string(settings.sentinels.size() - 1));
    }
    for (const auto& slave : slaves_) settings.sentinels.emplace_back(kLocalhost, slave->GetPort());

    sentinel_client_ = storages::redis::impl::Sentinel::CreateSentinel(
        thread_pools_,
        settings,
        "test_cluster_shard_group_name",
        dynamic_config::GetDefaultSource(),
        "test_cluster_client_name",
        std::move(key_shard)
    );
    sentinel_client_->WaitConnectedDebug(slaves_.empty());

    for (const auto& server : masters_) {
        EXPECT_TRUE(server->WaitForFirstPingReply(kSmallPeriod));
    }
    for (const auto& server : slaves_) {
        EXPECT_TRUE(server->WaitForFirstPingReply(kSmallPeriod));
    }
}

SentinelTest::SentinelTest(
    size_t sentinel_count,
    size_t master_count,
    size_t slave_count,
    int magic_value_add_master,
    int magic_value_add_slave,
    size_t redis_thread_count,
    storages::redis::impl::KeyShardFactory key_shard
)
    : masters_(InitServerArray(master_count, "masters", magic_value_add_master)),
      slaves_(InitServerArray(slave_count, "slaves", magic_value_add_slave)),
      sentinels_(InitServerArray(sentinel_count, "sentinels")),
      thread_pools_(std::make_shared<storages::redis::impl::ThreadPools>(1, redis_thread_count)) {
    UASSERT_MSG(!key_shard.IsClusterStrategy(), "Use SentinelTest");
    InitSentinelServers();
    CreateSentinelClient(std::move(key_shard));
}

void SentinelTest::InitSentinelServers() {
    std::vector<MockRedisServer::SlaveInfo> slave_infos;
    for (const auto& slave : slaves_) slave_infos.emplace_back(redis_name_, kLocalhost, slave->GetPort());
    for (auto& sentinel : sentinels_) {
        sentinel->RegisterSentinelMastersHandler({{redis_name_, kLocalhost, Master().GetPort()}});
        sentinel->RegisterSentinelSlavesHandler(redis_name_, slave_infos);
    }
}

void SentinelTest::CreateSentinelClient(storages::redis::impl::KeyShardFactory key_shard) {
    secdist::RedisSettings settings;
    UASSERT(!key_shard.IsClusterStrategy());
    settings.shards = {redis_name_};
    for (const auto& sentinel : sentinels_) settings.sentinels.emplace_back(kLocalhost, sentinel->GetPort());

    sentinel_client_ = storages::redis::impl::Sentinel::CreateSentinel(
        thread_pools_,
        settings,
        "test_shard_group_name",
        dynamic_config::GetDefaultSource(),
        "test_client_name",
        std::move(key_shard)
    );
    sentinel_client_->WaitConnectedDebug(slaves_.empty());

    for (const auto& sentinel : sentinels_) {
        EXPECT_TRUE(sentinel->WaitForFirstPingReply(kSmallPeriod));
    }
    sentinel_client_->WaitConnectedDebug(slaves_.empty());
}

SentinelShardTest::SentinelShardTest(
    size_t sentinel_count,
    size_t shard_count,
    int magic_value_add_master,
    int magic_value_add_slave,
    size_t redis_thread_count,
    storages::redis::impl::KeyShardFactory key_shard
)
    : redis_names_(InitRedisNames(shard_count)),
      masters_(InitServerArray(shard_count, "masters", magic_value_add_master)),
      slaves_(InitServerArray(shard_count, "slaves", magic_value_add_slave)),
      sentinels_(InitServerArray(sentinel_count, "sentinels")),
      thread_pools_(std::make_shared<storages::redis::impl::ThreadPools>(1, redis_thread_count)) {
    InitSentinelServers(shard_count);
    CreateSentinelClient(std::move(key_shard));
}

std::vector<std::string> SentinelShardTest::InitRedisNames(size_t shard_count) {
    assert(shard_count > 0);
    std::vector<std::string> result;
    for (size_t shard_idx = 0; shard_idx < shard_count; shard_idx++) {
        result.push_back("redis_name_" + std::to_string(shard_idx));
    }
    return result;
}

void SentinelShardTest::InitSentinelServers(size_t shard_count) {
    std::vector<MockRedisServer::MasterInfo> master_infos;
    for (size_t shard_idx = 0; shard_idx < shard_count; shard_idx++) {
        master_infos.emplace_back(redis_names_.at(shard_idx), kLocalhost, Master(shard_idx).GetPort());
    }
    for (auto& sentinel : sentinels_) {
        sentinel->RegisterSentinelMastersHandler(master_infos);
        for (size_t shard_idx = 0; shard_idx < shard_count; shard_idx++) {
            sentinel->RegisterSentinelSlavesHandler(
                redis_names_.at(shard_idx), {{redis_names_.at(shard_idx), kLocalhost, Slave(shard_idx).GetPort()}}
            );
        }
    }
}

void SentinelShardTest::CreateSentinelClient(storages::redis::impl::KeyShardFactory key_shard) {
    secdist::RedisSettings settings;
    settings.shards = redis_names_;
    for (const auto& sentinel : sentinels_) settings.sentinels.emplace_back(kLocalhost, sentinel->GetPort());

    sentinel_client_ = storages::redis::impl::Sentinel::CreateSentinel(
        thread_pools_,
        settings,
        "test_shard_group_name",
        dynamic_config::GetDefaultSource(),
        "test_client_name",
        std::move(key_shard)
    );
    sentinel_client_->WaitConnectedDebug(slaves_.empty());

    for (const auto& sentinel : sentinels_) {
        EXPECT_TRUE(sentinel->WaitForFirstPingReply(kSmallPeriod));
    }
}

USERVER_NAMESPACE_END
