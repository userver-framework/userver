#include "redis_fixture.hpp"

#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/storages/redis/impl/thread_pools.hpp>

#include <storages/redis/client_impl.hpp>
#include <storages/redis/impl/sentinel.hpp>
#include <storages/redis/redis_secdist.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::bench {

namespace {

constexpr std::size_t kMainWorkerThreads = 16;
constexpr std::size_t kSentinelThreadPoolSize = 1;
constexpr std::size_t kRedisThreadPoolSize = 3;

constexpr const char* kTestsuiteSentinelPort = "TESTSUITE_REDIS_SENTINEL_PORT";
constexpr const char* kDefaultSentinelPort = "26379";

constexpr std::string_view kRedisSettings = R"({{
  "redis_settings": {{
    "taxi-test": {{
        "command_control": {{
            "max_retries": 1,
            "timeout_all_ms": 1000,
            "timeout_single_ms": 1000
        }},
        "password": "",
        "sentinels": [{{"host": "localhost", "port": {}}}],
        "shards": [{{"name": "test_master0"}}]
    }}
  }}
}})";

const USERVER_NAMESPACE::secdist::RedisSettings& GetTestsuiteRedisSettings() {
  static const auto settings_map = [] {
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    const auto* sentinel_port_env = std::getenv(kTestsuiteSentinelPort);
    return storages::secdist::RedisMapSettings{formats::json::FromString(
        fmt::format(kRedisSettings, sentinel_port_env ? sentinel_port_env
                                                      : kDefaultSentinelPort))};
  }();
  return settings_map.GetSettings("taxi-test");
}

}  // namespace

void Redis::RunStandalone(std::function<void()> payload) {
  engine::RunStandalone(kMainWorkerThreads, [&] {
    auto thread_pools = std::make_shared<USERVER_NAMESPACE::redis::ThreadPools>(
        kSentinelThreadPoolSize, kRedisThreadPoolSize);
    dynamic_config::StorageMock config;

    sentinel_ = USERVER_NAMESPACE::redis::Sentinel::CreateSentinel(
        std::move(thread_pools), GetTestsuiteRedisSettings(), "none",
        config.GetSource(), "pub",
        USERVER_NAMESPACE::redis::KeyShardFactory{""});

    sentinel_->WaitConnectedDebug();
    sentinel_->MakeRequest({"FLUSHDB"}, "none").Get();

    client_ = std::make_shared<storages::redis::ClientImpl>(sentinel_);

    payload();

    client_.reset();
    sentinel_.reset();
  });
}

}  // namespace storages::redis::bench

USERVER_NAMESPACE_END
