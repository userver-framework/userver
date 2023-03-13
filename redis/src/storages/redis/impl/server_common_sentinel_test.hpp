#pragma once

#include <userver/storages/redis/impl/thread_pools.hpp>

#include <storages/redis/impl/mock_server_test.hpp>
#include <storages/redis/impl/sentinel.hpp>

USERVER_NAMESPACE_BEGIN

// 100ms should be enough, but valgrind is too slow
const auto kSmallPeriod = std::chrono::milliseconds(500);

const std::string kLocalhost = "127.0.0.1";

class SentinelTest {
 public:
  SentinelTest(size_t sentinel_count, size_t master_count, size_t slave_count,
               int magic_value_add_master = 0, int magic_value_add_slave = 0,
               size_t redis_thread_count = 1);

  using MockRedisServerArray = std::vector<std::unique_ptr<MockRedisServer>>;

  redis::Sentinel& SentinelClient() const { return *sentinel_client_; }

  MockRedisServerArray& Masters() { return masters_; }
  MockRedisServerArray& Slaves() { return slaves_; }
  MockRedisServerArray& Sentinels() { return sentinels_; }

  MockRedisServer& Master(size_t idx = 0) { return *masters_.at(idx); }
  MockRedisServer& Slave(size_t idx = 0) { return *slaves_.at(idx); }
  MockRedisServer& Sentinel(size_t idx = 0) { return *sentinels_.at(idx); }

  const std::string& RedisName() const { return redis_name_; }

 private:
  static MockRedisServerArray InitServerArray(
      size_t size, const std::string& description,
      std::optional<int> magic_value_add = {});
  void InitSentinelServers();
  void CreateSentinelClient();

  const std::string redis_name_{"redis_name"};
  MockRedisServerArray masters_;
  MockRedisServerArray slaves_;
  MockRedisServerArray sentinels_;

  std::shared_ptr<redis::ThreadPools> thread_pools_;
  std::shared_ptr<redis::Sentinel> sentinel_client_;
};

class SentinelShardTest {
 public:
  SentinelShardTest(size_t sentinel_count, size_t shard_count,
                    int magic_value_add_master = 0,
                    int magic_value_add_slave = 0,
                    size_t redis_thread_count = 1);

  using MockRedisServerArray = std::vector<std::unique_ptr<MockRedisServer>>;

  redis::Sentinel& SentinelClient() const { return *sentinel_client_; }

  MockRedisServerArray& Masters() { return masters_; }
  MockRedisServerArray& Slaves() { return slaves_; }
  MockRedisServerArray& Sentinels() { return sentinels_; }

  MockRedisServer& Master(size_t idx = 0) { return *masters_.at(idx); }
  MockRedisServer& Slave(size_t idx = 0) { return *slaves_.at(idx); }
  MockRedisServer& Sentinel(size_t idx = 0) { return *sentinels_.at(idx); }

  const std::string& RedisName(size_t idx) const {
    return redis_names_.at(idx);
  }

 private:
  static std::vector<std::string> InitRedisNames(size_t shard_count);
  static MockRedisServerArray InitServerArray(
      size_t size, const std::string& description,
      std::optional<int> magic_value_add = {});
  void InitSentinelServers(size_t shard_count);
  void CreateSentinelClient();

  const std::vector<std::string> redis_names_;
  MockRedisServerArray masters_;
  MockRedisServerArray slaves_;
  MockRedisServerArray sentinels_;

  std::shared_ptr<redis::ThreadPools> thread_pools_;
  std::shared_ptr<redis::Sentinel> sentinel_client_;
};

USERVER_NAMESPACE_END
