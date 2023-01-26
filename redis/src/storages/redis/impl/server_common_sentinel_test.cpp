#include "server_common_sentinel_test.hpp"

#include <userver/storages/redis/impl/secdist_redis.hpp>

USERVER_NAMESPACE_BEGIN

SentinelTest::SentinelTest(size_t sentinel_count, size_t master_count,
                           size_t slave_count, int magic_value_add_master,
                           int magic_value_add_slave, size_t redis_thread_count)
    : masters_(
          InitServerArray(master_count, "masters", magic_value_add_master)),
      slaves_(InitServerArray(slave_count, "slaves", magic_value_add_slave)),
      sentinels_(InitServerArray(sentinel_count, "sentinels")),
      thread_pools_(
          std::make_shared<redis::ThreadPools>(1, redis_thread_count)) {
  InitSentinelServers();
  CreateSentinelClient();
}

SentinelTest::MockRedisServerArray SentinelTest::InitServerArray(
    size_t size, const std::string& description,
    std::optional<int> magic_value_add) {
  MockRedisServerArray servers;
  for (size_t i = 0; i < size; i++) {
    servers.emplace_back(std::make_unique<MockRedisServer>(description + '-' +
                                                           std::to_string(i)));
    auto& server = *servers.back();
    server.RegisterPingHandler();
    if (magic_value_add)
      server.RegisterHandlerWithConstReply("GET", *magic_value_add + i);
    LOG_DEBUG() << description << '[' << i << "].port=" << server.GetPort();
  }
  return servers;
}

void SentinelTest::InitSentinelServers() {
  std::vector<MockRedisServer::SlaveInfo> slave_infos;
  for (const auto& slave : slaves_)
    slave_infos.emplace_back(redis_name_, kLocalhost, slave->GetPort());
  for (auto& sentinel : sentinels_) {
    sentinel->RegisterSentinelMastersHandler(
        {{redis_name_, kLocalhost, Master().GetPort()}});
    sentinel->RegisterSentinelSlavesHandler(redis_name_, slave_infos);
  }
}

void SentinelTest::CreateSentinelClient() {
  secdist::RedisSettings settings;
  settings.shards = {redis_name_};
  for (const auto& sentinel : sentinels_)
    settings.sentinels.emplace_back(kLocalhost, sentinel->GetPort());
  sentinel_client_ = redis::Sentinel::CreateSentinel(thread_pools_, settings,
                                                     "test_shard_group_name",
                                                     "test_client_name", {""});
  sentinel_client_->WaitConnectedDebug(slaves_.empty());

  for (const auto& sentinel : sentinels_) {
    EXPECT_TRUE(sentinel->WaitForFirstPingReply(kSmallPeriod));
  }
}

SentinelShardTest::SentinelShardTest(size_t sentinel_count, size_t shard_count,
                                     int magic_value_add_master,
                                     int magic_value_add_slave,
                                     size_t redis_thread_count)
    : redis_names_(InitRedisNames(shard_count)),
      masters_(InitServerArray(shard_count, "masters", magic_value_add_master)),
      slaves_(InitServerArray(shard_count, "slaves", magic_value_add_slave)),
      sentinels_(InitServerArray(sentinel_count, "sentinels")),
      thread_pools_(
          std::make_shared<redis::ThreadPools>(1, redis_thread_count)) {
  InitSentinelServers(shard_count);
  CreateSentinelClient();
}

std::vector<std::string> SentinelShardTest::InitRedisNames(size_t shard_count) {
  assert(shard_count > 0);
  std::vector<std::string> result;
  for (size_t shard_idx = 0; shard_idx < shard_count; shard_idx++) {
    result.push_back("redis_name_" + std::to_string(shard_idx));
  }
  return result;
}

SentinelShardTest::MockRedisServerArray SentinelShardTest::InitServerArray(
    size_t size, const std::string& description,
    std::optional<int> magic_value_add) {
  MockRedisServerArray servers;
  for (size_t i = 0; i < size; i++) {
    servers.emplace_back(std::make_unique<MockRedisServer>(description + '-' +
                                                           std::to_string(i)));
    auto& server = *servers.back();
    server.RegisterPingHandler();
    if (magic_value_add)
      server.RegisterHandlerWithConstReply("GET", *magic_value_add + i);
    LOG_DEBUG() << description << '[' << i << "].port=" << server.GetPort();
  }
  return servers;
}

void SentinelShardTest::InitSentinelServers(size_t shard_count) {
  std::vector<MockRedisServer::MasterInfo> master_infos;
  for (size_t shard_idx = 0; shard_idx < shard_count; shard_idx++) {
    master_infos.emplace_back(redis_names_.at(shard_idx), kLocalhost,
                              Master(shard_idx).GetPort());
  }
  for (auto& sentinel : sentinels_) {
    sentinel->RegisterSentinelMastersHandler(master_infos);
    for (size_t shard_idx = 0; shard_idx < shard_count; shard_idx++) {
      sentinel->RegisterSentinelSlavesHandler(
          redis_names_.at(shard_idx), {{redis_names_.at(shard_idx), kLocalhost,
                                        Slave(shard_idx).GetPort()}});
    }
  }
}

void SentinelShardTest::CreateSentinelClient() {
  secdist::RedisSettings settings;
  settings.shards = redis_names_;
  for (const auto& sentinel : sentinels_)
    settings.sentinels.emplace_back(kLocalhost, sentinel->GetPort());
  sentinel_client_ = redis::Sentinel::CreateSentinel(thread_pools_, settings,
                                                     "test_shard_group_name",
                                                     "test_client_name", {""});
  sentinel_client_->WaitConnectedDebug(slaves_.empty());

  for (const auto& sentinel : sentinels_) {
    EXPECT_TRUE(sentinel->WaitForFirstPingReply(kSmallPeriod));
  }
}

USERVER_NAMESPACE_END
