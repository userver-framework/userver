#include <storages/redis/util_benchmark.hpp>

#include <storages/redis/client_impl.hpp>
#include <storages/redis/redis_secdist.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/storages/redis/impl/sentinel.hpp>
#include <userver/storages/redis/impl/thread_pools.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::bench {

namespace {

constexpr const char* kTestsuiteSentinelPort = "TESTSUITE_REDIS_SENTINEL_PORT";
constexpr const char* kDefaultSentinelPort = "26379";

constexpr std::string_view kRedisSettings = R"({{
  "redis_settings": {{
    "taxi-test": {{
        "command_control": {{
            "max_retries": 1,
            "timeout_all_ms": 30000,
            "timeout_single_ms": 30000
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

void Redis::RunStandalone(std::size_t thread_count,
                          std::function<void()> payload) {
  engine::RunStandalone(thread_count, [&] {
    auto thread_pools = std::make_shared<USERVER_NAMESPACE::redis::ThreadPools>(
        USERVER_NAMESPACE::redis::kDefaultSentinelThreadPoolSize,
        USERVER_NAMESPACE::redis::kDefaultRedisThreadPoolSize);

    auto sentinel = USERVER_NAMESPACE::redis::Sentinel::CreateSentinel(
        std::move(thread_pools), GetTestsuiteRedisSettings(), "none", "pub",
        USERVER_NAMESPACE::redis::KeyShardFactory{""});

    sentinel->WaitConnectedDebug();
    sentinel->MakeRequest({"FLUSHDB"}, "none").Get();

    client_ =
        std::make_shared<storages::redis::ClientImpl>(std::move(sentinel));

    payload();

    client_.reset();
  });
}

}  // namespace storages::redis::bench

USERVER_NAMESPACE_END
