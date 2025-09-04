#include <storages/redis/utest/impl/redis_connection_state.hpp>

#include <fmt/format.h>

#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/engine/subprocess/environment_variables.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/utils/text.hpp>

#include <storages/redis/impl/keyshard_impl.hpp>
#include <storages/redis/impl/secdist_redis.hpp>
#include <storages/redis/redis_secdist.hpp>

USERVER_NAMESPACE_BEGIN

using ThreadPools = USERVER_NAMESPACE::storages::redis::impl::ThreadPools;
using Sentinel = USERVER_NAMESPACE::storages::redis::impl::Sentinel;
using SubscribeSentinel = USERVER_NAMESPACE::storages::redis::impl::SubscribeSentinel;
using KeyShardFactory = USERVER_NAMESPACE::storages::redis::impl::KeyShardFactory;

namespace storages::redis::utest::impl {

namespace {

constexpr const char* kTestsuiteSentinelPort = "TESTSUITE_REDIS_SENTINEL_PORT";
constexpr const char* kTestsuiteClusterRedisPorts = "TESTSUITE_REDIS_CLUSTER_PORTS";

constexpr std::string_view kRedisSettingsJsonFormat = R"(
{{
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
}}
)";

constexpr std::string_view kRedisClusterSettingsJsonFormat = R"(
{{
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
}}
)";

const USERVER_NAMESPACE::secdist::RedisSettings& GetRedisSettings() {
    static const auto settings_map = [] {
        constexpr const char* kDefaultSentinelPort = "26379";
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        const auto* sentinel_port_env = std::getenv(kTestsuiteSentinelPort);
        return storages::secdist::RedisMapSettings{formats::json::FromString(
            fmt::format(kRedisSettingsJsonFormat, sentinel_port_env ? sentinel_port_env : kDefaultSentinelPort)
        )};
    }();

    return settings_map.GetSettings("taxi-test");
}

const USERVER_NAMESPACE::secdist::RedisSettings& GetRedisClusterSettings() {
    static const auto settings_map = []() {
        const auto port_strings = []() -> std::vector<std::string> {
            static const auto kDefaultPorts =
                std::vector<std::string>{"17380", "17381", "17382", "17383", "17384", "17385"};
            const auto env = engine::subprocess::GetCurrentEnvironmentVariables();
            const auto* cluster_ports = env.GetValueOptional(kTestsuiteClusterRedisPorts);
            if (!cluster_ports) {
                return kDefaultPorts;
            }

            auto ret = utils::text::Split(*cluster_ports, ",");
            UINVARIANT(
                ret.size() == 6,
                fmt::format(
                    "Expecting {} to have 6 comma separated variables. Instead it contains: {}",
                    kTestsuiteClusterRedisPorts,
                    *cluster_ports
                )
            );
            return ret;
        }();

        auto json_config = fmt::format(
            kRedisClusterSettingsJsonFormat,
            port_strings[0],
            port_strings[1],
            port_strings[2],
            port_strings[3],
            port_strings[4],
            port_strings[5]
        );
        return storages::secdist::RedisMapSettings{formats::json::FromString(json_config)};
    }();

    return settings_map.GetSettings("cluster-test");
}

dynamic_config::StorageMock MakeRedisDynamicConfigStorage() {
    auto docs_map = dynamic_config::impl::GetDefaultDocsMap();
    docs_map.Set("REDIS_REPLICA_MONITORING_SETTINGS", formats::json::FromString(R"({
      "__default__": {
        "enable-monitoring": true,
        "forbid-requests-to-syncing-replicas": true
      }
    })"));
    docs_map.Set("REDIS_WAIT_CONNECTED", formats::json::FromString(R"({
      "mode": "master_or_slave",
      "throw_on_fail": true,
      "timeout-ms": 20000
    })"));
    return dynamic_config::StorageMock(docs_map, {});
}

dynamic_config::Source GetRedisDynamicConfigSource() {
    static auto storage = MakeRedisDynamicConfigStorage();
    return storage.GetSource();
}

}  // namespace

RedisConnectionState::RedisConnectionState() {
    auto configs_source = GetRedisDynamicConfigSource();

    thread_pools_ = std::make_shared<ThreadPools>(
        USERVER_NAMESPACE::storages::redis::impl::kDefaultSentinelThreadPoolSize,
        USERVER_NAMESPACE::storages::redis::impl::kDefaultRedisThreadPoolSize
    );

    sentinel_ =
        Sentinel::CreateSentinel(thread_pools_, GetRedisSettings(), "none", configs_source, "pub", KeyShardFactory{""});
    sentinel_->WaitConnectedDebug();
    client_ = std::make_shared<ClientImpl>(sentinel_);

    subscribe_sentinel_ = SubscribeSentinel::Create(
        thread_pools_, GetRedisSettings(), "none", configs_source, "pub", "KeyShardZero", {}, {}
    );
    subscribe_sentinel_->WaitConnectedDebug();
    subscribe_client_ = std::make_shared<SubscribeClientImpl>(subscribe_sentinel_);
}

RedisConnectionState::RedisConnectionState(InClusterMode) {
    auto configs_source = GetRedisDynamicConfigSource();

    thread_pools_ = std::make_shared<ThreadPools>(
        USERVER_NAMESPACE::storages::redis::impl::kDefaultSentinelThreadPoolSize,
        USERVER_NAMESPACE::storages::redis::impl::kDefaultRedisThreadPoolSize
    );

    sentinel_ = Sentinel::CreateSentinel(
        thread_pools_,
        GetRedisClusterSettings(),
        "none",
        configs_source,
        "pub",
        KeyShardFactory{storages::redis::impl::kRedisCluster}
    );
    sentinel_->WaitConnectedDebug();
    UASSERT(sentinel_->ShardsCount() != 0);

    client_ = std::make_shared<ClientImpl>(sentinel_);

    subscribe_sentinel_ = storages::redis::impl::SubscribeSentinel::Create(
        thread_pools_, GetRedisClusterSettings(), "none", configs_source, "pub", "RedisCluster", {}, {}
    );
    subscribe_sentinel_->WaitConnectedDebug();
    subscribe_client_ = std::make_shared<storages::redis::SubscribeClientImpl>(subscribe_sentinel_);
}

}  // namespace storages::redis::utest::impl

USERVER_NAMESPACE_END
