#include <userver/components/minimal_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <userver/utest/parameter_names.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

auto MakeRedisConfig(const std::string& sharding_strategy) {
    // BEWARE! No separate fs-task-processor. Testing almost single thread mode
    return components::InMemoryConfig{R"(
components_manager:
  coro_pool:
    initial_size: 50
  default_task_processor: main-task-processor
  fs_task_processor: main-task-processor
  event_thread_pool:
    threads: 1
  task_processors:
    main-task-processor:
      worker_threads: 1
  components:
    logging:
      fs-task-processor: main-task-processor
      loggers:
        default:
          file_path: '@null'
    testsuite-support: {}
    secdist: {}
    default-secdist-provider:
        missing-ok: true
        inline:
          'redis_settings': {
            'redis-db': {
                'password': '',
                'sentinels': [{'host': '0.0.0.0', 'port': 1}],
                'shards': [{'name': 'unexisting_shard'}]
            }
          }
    dynamic-config:
      defaults:
        REDIS_WAIT_CONNECTED: {
          "mode": "master_or_slave",
          "throw_on_fail": true,
          "timeout-ms": 100
        }
    redis:
      subscribe_groups: []
      thread_pools:
          redis_thread_pool_size: 2
          sentinel_thread_pool_size: 1
      groups:
        - config_name: redis-db
          db: redis-db
          sharding_strategy: )" + sharding_strategy};
}

auto MakeRedisComponentList() {
    return components::MinimalComponentList()
        .Append<components::Redis>("redis")
        .Append<components::TestsuiteSupport>()
        .Append<components::Secdist>()
        .Append<components::DefaultSecdistProvider>();
}

}  // namespace

TEST(RedisComponentBadConfig, RedisStandalone) {
    UEXPECT_THROW_MSG(
        components::RunOnce(MakeRedisConfig("RedisStandalone"), MakeRedisComponentList()),
        std::exception,
        "Cannot start component redis: Failed to init cluster slots for redis, shard_group_name=redis-db in 100 ms, "
        "mode=master_or_slave. Nodes config parsed: true. Shards readiness: [{master: false, replicas: false},]. "
        "Failed to connect to the Redis shards"
    );
}

TEST(RedisComponentBadConfig, RedisCluster) {
    UEXPECT_THROW_MSG(
        components::RunOnce(MakeRedisConfig("RedisCluster"), MakeRedisComponentList()),
        std::exception,
        "Cannot start component redis: Failed to init cluster slots for redis, shard_group_name=redis-db in 100 ms, "
        "mode=master_or_slave. Nodes received: false; topology received: false. Shards readiness: []. "
        "Failed to connect to the Redis shards"
    );
}

TEST(RedisComponentBadConfig, KeyShardCrc32) {
    UEXPECT_THROW_MSG(
        components::RunOnce(MakeRedisConfig("KeyShardCrc32"), MakeRedisComponentList()),
        std::exception,
        "Cannot start component redis: Failed to connect to redis, shard_group_name=redis-db, shard=unexisting_shard "
        "in 100 ms, mode=master_or_slave"
    );
}

USERVER_NAMESPACE_END
