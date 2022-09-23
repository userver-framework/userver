#include <storages/redis/util_redistest.hpp>

#include <cstdlib>

#include <fmt/format.h>

#include <storages/redis/redis_secdist.hpp>
#include <userver/formats/json/serialize.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr const char* kTestsuiteSentinelPort = "TESTSUITE_REDIS_SENTINEL_PORT";
constexpr const char* kDefaultSentinelPort = "26379";

constexpr std::string_view kRedisSettingsJsonFormat = R"({{
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

const std::string kRedisClusterSettingsJsonFormat = R"({
  "redis_settings": {
    "cluster-test": {
      "password": "",
      "sentinels": [
        {"host": "localhost", "port": 7000},
        {"host": "localhost", "port": 7001},
        {"host": "localhost", "port": 7002},
        {"host": "localhost", "port": 7003},
        {"host": "localhost", "port": 7004},
        {"host": "localhost", "port": 7005},
        {"host": "localhost", "port": 7006},
        {"host": "localhost", "port": 7007},
        {"host": "localhost", "port": 7008}
      ],
      "shards": [
        {"name": "master0"},
        {"name": "master1"},
        {"name": "master2"}
      ]
    }
  }
})";

}  // namespace

const secdist::RedisSettings& GetTestsuiteRedisSettings() {
  static const auto settings_map = [] {
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    const auto* sentinel_port_env = std::getenv(kTestsuiteSentinelPort);
    return storages::secdist::RedisMapSettings{
        formats::json::FromString(fmt::format(
            kRedisSettingsJsonFormat,
            sentinel_port_env ? sentinel_port_env : kDefaultSentinelPort))};
  }();
  return settings_map.GetSettings("taxi-test");
}

const secdist::RedisSettings& GetTestsuiteRedisClusterSettings() {
  static const auto settings_map = storages::secdist::RedisMapSettings{
      formats::json::FromString(kRedisClusterSettingsJsonFormat)};
  return settings_map.GetSettings("cluster-test");
}

USERVER_NAMESPACE_END
