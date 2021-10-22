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

}  // namespace

const secdist::RedisSettings& GetTestsuiteRedisSettings() {
  static const auto settings_map = [] {
    const auto* sentinel_port_env = std::getenv(kTestsuiteSentinelPort);
    return storages::secdist::RedisMapSettings{
        formats::json::FromString(fmt::format(
            kRedisSettingsJsonFormat,
            sentinel_port_env ? sentinel_port_env : kDefaultSentinelPort))};
  }();
  return settings_map.GetSettings("taxi-test");
}

USERVER_NAMESPACE_END
