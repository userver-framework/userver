#include <storages/redis/util_redistest.hpp>

#include <cstdlib>
#include <string_view>

#include <fmt/format.h>

#include <storages/redis/redis_secdist.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/utils/text.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr const char* kTestsuiteSentinelPort = "TESTSUITE_REDIS_SENTINEL_PORT";
constexpr const char* kDefaultSentinelPort = "26379";
constexpr const char* kTestsuiteClusterRedisPorts =
    "TESTSUITE_REDIS_CLUSTER_PORTS";

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

constexpr std::string_view kRedisClusterSettingsJsonFormat = R"({{
  "redis_settings": {{
    "cluster-test": {{
      "password": "",
      "sentinels": [
        {{"host": "::1", "port": {}}},
        {{"host": "::1", "port": {}}},
        {{"host": "::1", "port": {}}},
        {{"host": "::1", "port": {}}},
        {{"host": "::1", "port": {}}},
        {{"host": "::1", "port": {}}}
      ],
      "shards": [
        {{"name": "master0"}},
        {{"name": "master1"}},
        {{"name": "master2"}}
      ]
    }}
  }}
}})";

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
  static const auto settings_map = []() {
    const auto port_strings = []() -> std::vector<std::string> {
      static const auto kDefaultPorts = std::vector<std::string>{
          "17380", "17381", "17382", "17383", "17384", "17385"};
      // NOLINTNEXTLINE(concurrency-mt-unsafe)
      const auto* cluster_ports = std::getenv(kTestsuiteClusterRedisPorts);
      if (!cluster_ports) {
        return kDefaultPorts;
      }
      auto ret = utils::text::Split(cluster_ports, ",");
      if (ret.size() != 6) {
        return kDefaultPorts;
      }
      return ret;
    }();
    return storages::secdist::RedisMapSettings{formats::json::FromString(
        fmt::format(kRedisClusterSettingsJsonFormat, port_strings[0],
                    port_strings[1], port_strings[2], port_strings[3],
                    port_strings[4], port_strings[5]))};
  }();

  return settings_map.GetSettings("cluster-test");
}

USERVER_NAMESPACE_END
