#include "mock_server_test.hpp"

#include <thread>

#include <userver/engine/single_consumer_event.hpp>
#include <userver/storages/redis/base.hpp>

#include <storages/redis/impl/command.hpp>
#include <storages/redis/impl/redis_group.hpp>
#include <storages/redis/impl/secdist_redis.hpp>
#include <storages/redis/impl/sentinel.hpp>
#include <storages/redis/impl/subscribe_sentinel.hpp>
#include <storages/redis/impl/thread_pools.hpp>
#include <storages/redis/subscribe_client_impl.hpp>
#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/storages/redis/subscribe_client.hpp>
#include <userver/storages/redis/subscription_token.hpp>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

// kSuccessTimeout is used only for waits that are expected to succeed
// (EXPECT_TRUE(...WaitForFirstReply...)). WaitForFirstReply returns as soon as
// the awaited reply arrives, so a generous timeout does not slow down the happy
// path while preventing flaps under CI/sanitizer load (the redis ping interval
// alone is 2000ms). Checks that something must NOT happen use kWaitPeriod.
constexpr auto kSuccessTimeout = utest::kMaxTestWaitTime;
constexpr std::chrono::milliseconds kWaitPeriod{10};
constexpr auto kWaitRetries = 100;
constexpr auto kCheckCount = 10;
constexpr auto kRedisDatabaseIndex = 46;
constexpr std::size_t kDatabaseIndex = 0;

const std::string kDbName = "redis_db";
const std::string kLocalhost = "127.0.0.1";

template <typename Predicate>
void PeriodicCheck(Predicate predicate) {
    for (int i = 0; i < kCheckCount; i++) {
        EXPECT_TRUE(predicate());
        std::this_thread::sleep_for(kWaitPeriod);
    }
}

template <typename Predicate>
void PeriodicWait(Predicate predicate) {
    for (int i = 0; i < kWaitRetries; i++) {
        if (predicate()) {
            break;
        }
        std::this_thread::sleep_for(kWaitPeriod);
    }
    EXPECT_TRUE(predicate());
}

bool IsConnected(const storages::redis::impl::Redis& redis) {
    return redis.GetState() == storages::redis::RedisState::kConnected;
}

struct MockSentinelServers {
    static constexpr size_t kRedisThreadCount = 1;
    static constexpr std::string_view kRedisName = "redis_name";

    void RegisterSentinelMastersSlaves() {
        std::vector<MockRedisServer::SlaveInfo> slave_infos;
        std::string redis_name{kRedisName};
        for (const auto& slave : slaves) {
            slave_infos.emplace_back(redis_name, kLocalhost, slave.GetPort());
        }

        for (auto& sentinel : sentinels) {
            sentinel.RegisterSentinelMastersHandler({{redis_name, kLocalhost, masters[0].GetPort()}});
            sentinel.RegisterSentinelSlavesHandler(redis_name, slave_infos);
        }
    }

    template <class Function>
    void ForEachServer(const Function& visitor) {
        for (auto& server : masters) {
            visitor(server);
        }
        for (auto& server : slaves) {
            visitor(server);
        }
        for (auto& server : sentinels) {
            visitor(server);
        }
    }

    void CreateSentinelClientAndWait(const secdist::RedisSettings& settings) const {
        auto sentinel_client = storages::redis::impl::Sentinel::CreateSentinel(
            thread_pool,
            settings,
            "test_shard_group_name",
            dynamic_config::GetDefaultSource(),
            storages::redis::impl::SentinelStaticConfig{
                "test_client_name",
                {storages::redis::ShardingStrategy::kKeyShardTaximeterCrc32},
                {},
                {},
            }
        );
        sentinel_client->WaitConnectedDebug(std::empty(slaves));
    }

    void CreateSubscribeSentinelClientAndWait(const secdist::RedisSettings& settings) {
        // Sentinels do NOT receive SUBSCRIBE
        std::vector<MockRedisServer::HandlerPtr> subscribe_handlers;
        for (auto& server : masters) {
            subscribe_handlers.push_back(server.RegisterHandlerWithConstReply("SUBSCRIBE", 1));
        }
        for (auto& server : slaves) {
            subscribe_handlers.push_back(server.RegisterHandlerWithConstReply("SUBSCRIBE", 1));
        }

        auto dynconf = dynamic_config::GetDefaultSource();
        using storages::redis::impl::SubscribeSentinel;
        auto subscribe_sentinel = SubscribeSentinel::Create(
            thread_pool,
            settings,
            "test_shard_group_name",
            dynconf,
            storages::redis::impl::SubscribeSentinelStaticConfig{
                "test_client_name",
                {storages::redis::ShardingStrategy::kKeyShardTaximeterCrc32},
                {},
                {},
            }
        );
        subscribe_sentinel->WaitConnectedDebug(std::empty(slaves));

        std::shared_ptr<storages::redis::SubscribeClient>
            client = std::make_shared<storages::redis::SubscribeClientImpl>(std::move(subscribe_sentinel));

        storages::redis::SubscriptionToken::OnMessageCb callback =
            [](const std::string& channel, const std::string& message) {
                EXPECT_TRUE(false) << "Should not be called. Channel = " << channel << ", message = " << message;
            };
        auto subscription = client->Subscribe("channel_name", std::move(callback));

        for (auto& handler : subscribe_handlers) {
            EXPECT_TRUE(handler->WaitForFirstReply(utest::kMaxTestWaitTime));
        }
    }

    MockRedisServer masters[1] = {
        MockRedisServer{"master0"},
    };
    MockRedisServer slaves[2] = {
        MockRedisServer{"slave0"},
        MockRedisServer{"slave1"},
    };
    MockRedisServer sentinels[3] = {
        MockRedisServer{"sentinel0"},
        MockRedisServer{"sentinel1"},
        MockRedisServer{"sentinel2"},
    };
    std::shared_ptr<storages::redis::impl::ThreadPools>
        thread_pool = std::make_shared<storages::redis::impl::ThreadPools>(1, kRedisThreadCount);
};

}  // namespace

UTEST(Redis, NoPassword) {
    MockRedisServer server{kDbName};
    auto ping_handler = server.RegisterPingHandler();

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    auto redis = std::make_shared<
        storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings, kDbName, stats);
    redis->Connect(
        {kLocalhost},
        server.GetPort(),
        storages::redis::Credentials{"", storages::redis::Password("")},
        kDatabaseIndex
    );

    EXPECT_TRUE(ping_handler->WaitForFirstReply(kSuccessTimeout));
}

UTEST(Redis, Auth) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto auth_handler = server.RegisterStatusReplyHandler("AUTH", {"password"}, "OK");

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    auto redis = std::make_shared<
        storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings, kDbName, stats);
    redis->Connect(
        {kLocalhost},
        server.GetPort(),
        storages::redis::Credentials{"", storages::redis::Password("password")},
        kDatabaseIndex
    );

    EXPECT_TRUE(auth_handler->WaitForFirstReply(kSuccessTimeout));
    EXPECT_TRUE(ping_handler->WaitForFirstReply(kSuccessTimeout));
}

UTEST(Redis, AuthWithUsername) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto auth_handler = server.RegisterStatusReplyHandler("AUTH", {"username", "password"}, "OK");

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    auto redis = std::make_shared<
        storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings, kDbName, stats);
    redis->Connect(
        {kLocalhost},
        server.GetPort(),
        storages::redis::Credentials{"username", storages::redis::Password("password")},
        kDatabaseIndex
    );

    EXPECT_TRUE(auth_handler->WaitForFirstReply(kSuccessTimeout));
    EXPECT_TRUE(ping_handler->WaitForFirstReply(kSuccessTimeout));
}

UTEST(Redis, AuthFail) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto auth_error_handler = server.RegisterErrorReplyHandler("AUTH", "NO PASARAN");

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    auto redis = std::make_shared<
        storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings, kDbName, stats);
    redis->Connect(
        {kLocalhost},
        server.GetPort(),
        storages::redis::Credentials{"", storages::redis::Password("password")},
        kDatabaseIndex
    );

    EXPECT_TRUE(auth_error_handler->WaitForFirstReply(kSuccessTimeout));
    PeriodicCheck([&] { return !IsConnected(*redis); });
}

UTEST(Redis, AuthTimeout) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto sleep_period = storages::redis::kDefaultTimeoutSingle + std::chrono::milliseconds(30);
    auto auth_error_handler = server.RegisterTimeoutHandler("AUTH", sleep_period);

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    auto redis = std::make_shared<
        storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings, kDbName, stats);
    redis->Connect(
        {kLocalhost},
        server.GetPort(),
        storages::redis::Credentials{"", storages::redis::Password("password")},
        kDatabaseIndex
    );

    EXPECT_TRUE(auth_error_handler->WaitForFirstReply(sleep_period + kSuccessTimeout));
    PeriodicCheck([&] { return !IsConnected(*redis); });
}

UTEST(Redis, SentinelAuth) {
    MockSentinelServers mock;
    mock.RegisterSentinelMastersSlaves();
    mock.ForEachServer([](auto& server) { server.RegisterPingHandler(); });
    auto& [masters, slaves, sentinels, thread_pool] = mock;

    secdist::RedisSettings settings;
    settings.shards = {std::string{MockSentinelServers::kRedisName}};
    settings.sentinel_password = storages::redis::Password("sentinel_password");
    settings.sentinels.reserve(std::size(sentinels));
    for (const auto& sentinel : sentinels) {
        settings.sentinels.emplace_back(kLocalhost, sentinel.GetPort());
    }

    std::vector<MockRedisServer::HandlerPtr> auth_handlers;
    auth_handlers.reserve(std::size(sentinels));
    for (auto& sentinel : sentinels) {
        auth_handlers.push_back(sentinel.RegisterStatusReplyHandler("AUTH", {"sentinel_password"}, "OK"));
    }
    std::vector<MockRedisServer::HandlerPtr> no_auth_handlers;
    no_auth_handlers.reserve(std::size(masters) + std::size(slaves));
    for (auto& server : masters) {
        no_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", "FAIL"));
    }
    for (auto& server : slaves) {
        no_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", "FAIL"));
    }

    mock.CreateSentinelClientAndWait(settings);

    for (auto& handler : auth_handlers) {
        EXPECT_TRUE(handler->WaitForFirstReply(kSuccessTimeout));
    }

    for (auto& handler : no_auth_handlers) {
        EXPECT_FALSE(handler->WaitForFirstReply(kWaitPeriod));
    }

    for (const auto& sentinel : sentinels) {
        EXPECT_TRUE(sentinel.WaitForFirstPingReply(kSuccessTimeout));
    }
}

UTEST(Redis, SentinelAuthWithUsername) {
    MockSentinelServers mock;
    mock.RegisterSentinelMastersSlaves();
    mock.ForEachServer([](auto& server) { server.RegisterPingHandler(); });
    auto& [masters, slaves, sentinels, thread_pool] = mock;

    secdist::RedisSettings settings;
    settings.shards = {std::string{MockSentinelServers::kRedisName}};
    settings.sentinel_username = "sentinel_username";
    settings.sentinel_password = storages::redis::Password("sentinel_password");
    settings.sentinels.reserve(std::size(sentinels));
    for (const auto& sentinel : sentinels) {
        settings.sentinels.emplace_back(kLocalhost, sentinel.GetPort());
    }

    std::vector<MockRedisServer::HandlerPtr> auth_handlers;
    auth_handlers.reserve(std::size(sentinels));
    for (auto& sentinel : sentinels) {
        auth_handlers
            .push_back(sentinel.RegisterStatusReplyHandler("AUTH", {"sentinel_username", "sentinel_password"}, "OK"));
    }
    std::vector<MockRedisServer::HandlerPtr> no_auth_handlers;
    no_auth_handlers.reserve(std::size(masters) + std::size(slaves));
    for (auto& server : masters) {
        no_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", "FAIL"));
    }
    for (auto& server : slaves) {
        no_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", "FAIL"));
    }

    mock.CreateSentinelClientAndWait(settings);

    for (auto& handler : auth_handlers) {
        EXPECT_TRUE(handler->WaitForFirstReply(kSuccessTimeout));
    }

    for (auto& handler : no_auth_handlers) {
        EXPECT_FALSE(handler->WaitForFirstReply(kWaitPeriod));
    }

    for (const auto& sentinel : sentinels) {
        EXPECT_TRUE(sentinel.WaitForFirstPingReply(kSuccessTimeout));
    }
}

UTEST(Redis, SentinelAuthWithUsernameAndPassword) {
    MockSentinelServers mock;
    mock.RegisterSentinelMastersSlaves();
    mock.ForEachServer([](auto& server) { server.RegisterPingHandler(); });
    auto& [masters, slaves, sentinels, thread_pool] = mock;

    secdist::RedisSettings settings;
    settings.shards = {std::string{MockSentinelServers::kRedisName}};
    settings.username = "username";
    settings.password = storages::redis::Password("password");
    settings.sentinel_username = "sentinel_username";
    settings.sentinel_password = storages::redis::Password("sentinel_password");
    settings.sentinels.reserve(std::size(sentinels));
    for (const auto& sentinel : sentinels) {
        settings.sentinels.emplace_back(kLocalhost, sentinel.GetPort());
    }

    std::vector<MockRedisServer::HandlerPtr> auth_handlers;
    auth_handlers.reserve(std::size(sentinels));
    for (auto& sentinel : sentinels) {
        auth_handlers
            .push_back(sentinel.RegisterStatusReplyHandler("AUTH", {"sentinel_username", "sentinel_password"}, "OK"));
    }
    std::vector<MockRedisServer::HandlerPtr> data_auth_handlers;
    data_auth_handlers.reserve(std::size(masters) + std::size(slaves));
    for (auto& server : masters) {
        data_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", {"username", "password"}, "FAIL"));
    }
    for (auto& server : slaves) {
        data_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", {"username", "password"}, "FAIL"));
    }

    mock.CreateSentinelClientAndWait(settings);

    for (auto& handler : auth_handlers) {
        EXPECT_TRUE(handler->WaitForFirstReply(kSuccessTimeout));
    }

    for (auto& handler : data_auth_handlers) {
        EXPECT_TRUE(handler->WaitForFirstReply(kWaitPeriod));
    }

    for (const auto& sentinel : sentinels) {
        EXPECT_TRUE(sentinel.WaitForFirstPingReply(kSuccessTimeout));
    }
}

UTEST(Redis, SentinelNoAuthButPassword) {
    MockSentinelServers mock;
    mock.RegisterSentinelMastersSlaves();
    mock.ForEachServer([](auto& server) { server.RegisterPingHandler(); });
    auto& [masters, slaves, sentinels, thread_pool] = mock;

    secdist::RedisSettings settings;
    settings.shards = {std::string{MockSentinelServers::kRedisName}};
    settings.password = storages::redis::Password("password");
    settings.sentinels.reserve(std::size(sentinels));
    for (const auto& sentinel : sentinels) {
        settings.sentinels.emplace_back(kLocalhost, sentinel.GetPort());
    }

    std::vector<MockRedisServer::HandlerPtr> no_auth_handlers;
    no_auth_handlers.reserve(std::size(sentinels));
    for (auto& sentinel : sentinels) {
        no_auth_handlers.push_back(sentinel.RegisterStatusReplyHandler("AUTH", "FAIL"));
    }
    std::vector<MockRedisServer::HandlerPtr> auth_handlers;
    auth_handlers.reserve(std::size(masters) + std::size(slaves));
    for (auto& server : masters) {
        auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", {"password"}, "OK"));
    }
    for (auto& server : slaves) {
        auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", {"password"}, "OK"));
    }

    mock.CreateSentinelClientAndWait(settings);

    for (auto& handler : auth_handlers) {
        EXPECT_TRUE(handler->WaitForFirstReply(kSuccessTimeout));
    }

    for (auto& handler : no_auth_handlers) {
        EXPECT_FALSE(handler->WaitForFirstReply(kWaitPeriod));
    }

    for (const auto& sentinel : sentinels) {
        EXPECT_TRUE(sentinel.WaitForFirstPingReply(kSuccessTimeout));
    }
}

UTEST(Redis, SentinelNoAuth) {
    MockSentinelServers mock;
    mock.RegisterSentinelMastersSlaves();
    mock.ForEachServer([](auto& server) { server.RegisterPingHandler(); });
    auto& [masters, slaves, sentinels, thread_pool] = mock;

    secdist::RedisSettings settings;
    settings.shards = {std::string{MockSentinelServers::kRedisName}};
    settings.sentinels.reserve(std::size(sentinels));
    for (const auto& sentinel : sentinels) {
        settings.sentinels.emplace_back(kLocalhost, sentinel.GetPort());
    }

    std::vector<MockRedisServer::HandlerPtr> no_auth_handlers;
    no_auth_handlers.reserve(std::size(sentinels) + std::size(masters) + std::size(slaves));
    mock.ForEachServer([&](auto& server) {
        no_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", "FAIL"));
    });

    mock.CreateSentinelClientAndWait(settings);

    for (auto& handler : no_auth_handlers) {
        EXPECT_FALSE(handler->WaitForFirstReply(kWaitPeriod));
    }

    for (const auto& sentinel : sentinels) {
        EXPECT_TRUE(sentinel.WaitForFirstPingReply(kSuccessTimeout));
    }
}

UTEST(Redis, SentinelAuthSubscribe) {
    MockSentinelServers mock;
    mock.RegisterSentinelMastersSlaves();
    mock.ForEachServer([](auto& server) { server.RegisterPingHandler(); });
    auto& [masters, slaves, sentinels, thread_pool] = mock;

    secdist::RedisSettings settings;
    settings.shards = {std::string{MockSentinelServers::kRedisName}};
    settings.sentinel_password = storages::redis::Password("sentinel_password");
    settings.sentinels.reserve(std::size(sentinels));
    for (const auto& sentinel : sentinels) {
        settings.sentinels.emplace_back(kLocalhost, sentinel.GetPort());
    }

    std::vector<MockRedisServer::HandlerPtr> auth_handlers;
    auth_handlers.reserve(std::size(sentinels));
    for (auto& sentinel : sentinels) {
        auth_handlers.push_back(sentinel.RegisterStatusReplyHandler("AUTH", {"sentinel_password"}, "OK"));
    }
    std::vector<MockRedisServer::HandlerPtr> no_auth_handlers;
    no_auth_handlers.reserve(std::size(masters) + std::size(slaves));
    for (auto& server : masters) {
        no_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", "FAIL"));
    }
    for (auto& server : slaves) {
        no_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", "FAIL"));
    }

    mock.CreateSubscribeSentinelClientAndWait(settings);

    for (auto& handler : auth_handlers) {
        EXPECT_TRUE(handler->WaitForFirstReply(kSuccessTimeout));
    }

    for (auto& handler : no_auth_handlers) {
        EXPECT_FALSE(handler->WaitForFirstReply(kWaitPeriod));
    }
}

UTEST(Redis, SentinelAuthSubscribeWithUsername) {
    MockSentinelServers mock;
    mock.RegisterSentinelMastersSlaves();
    mock.ForEachServer([](auto& server) { server.RegisterPingHandler(); });
    auto& [masters, slaves, sentinels, thread_pool] = mock;

    secdist::RedisSettings settings;
    settings.shards = {std::string{MockSentinelServers::kRedisName}};
    settings.sentinel_username = "sentinel_username";
    settings.sentinel_password = storages::redis::Password("sentinel_password");
    settings.sentinels.reserve(std::size(sentinels));
    for (const auto& sentinel : sentinels) {
        settings.sentinels.emplace_back(kLocalhost, sentinel.GetPort());
    }

    std::vector<MockRedisServer::HandlerPtr> auth_handlers;
    auth_handlers.reserve(std::size(sentinels));
    for (auto& sentinel : sentinels) {
        auth_handlers
            .push_back(sentinel.RegisterStatusReplyHandler("AUTH", {"sentinel_username", "sentinel_password"}, "OK"));
    }
    std::vector<MockRedisServer::HandlerPtr> no_auth_handlers;
    no_auth_handlers.reserve(std::size(masters) + std::size(slaves));
    for (auto& server : masters) {
        no_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", "FAIL"));
    }
    for (auto& server : slaves) {
        no_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", "FAIL"));
    }

    mock.CreateSubscribeSentinelClientAndWait(settings);

    for (auto& handler : auth_handlers) {
        EXPECT_TRUE(handler->WaitForFirstReply(kSuccessTimeout));
    }

    for (auto& handler : no_auth_handlers) {
        EXPECT_FALSE(handler->WaitForFirstReply(kWaitPeriod));
    }
}

UTEST(Redis, SentinelNoAuthSubscribeButPassword) {
    MockSentinelServers mock;
    mock.RegisterSentinelMastersSlaves();
    mock.ForEachServer([](auto& server) { server.RegisterPingHandler(); });
    auto& [masters, slaves, sentinels, thread_pool] = mock;

    secdist::RedisSettings settings;
    settings.shards = {std::string{MockSentinelServers::kRedisName}};
    settings.password = storages::redis::Password("password");
    settings.sentinels.reserve(std::size(sentinels));
    for (const auto& sentinel : sentinels) {
        settings.sentinels.emplace_back(kLocalhost, sentinel.GetPort());
    }

    std::vector<MockRedisServer::HandlerPtr> no_auth_handlers;
    no_auth_handlers.reserve(std::size(sentinels));
    for (auto& sentinel : sentinels) {
        no_auth_handlers.push_back(sentinel.RegisterStatusReplyHandler("AUTH", "FAIL"));
    }
    std::vector<MockRedisServer::HandlerPtr> auth_handlers;
    auth_handlers.reserve(std::size(masters) + std::size(slaves));
    for (auto& server : masters) {
        auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", {"password"}, "OK"));
    }
    for (auto& server : slaves) {
        auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", {"password"}, "OK"));
    }

    mock.CreateSubscribeSentinelClientAndWait(settings);

    for (auto& handler : auth_handlers) {
        EXPECT_TRUE(handler->WaitForFirstReply(kSuccessTimeout));
    }

    for (auto& handler : no_auth_handlers) {
        EXPECT_FALSE(handler->WaitForFirstReply(kWaitPeriod));
    }
}

UTEST(Redis, SentinelNoAuthSubscribe) {
    MockSentinelServers mock;
    mock.RegisterSentinelMastersSlaves();
    mock.ForEachServer([](auto& server) { server.RegisterPingHandler(); });
    auto& [masters, slaves, sentinels, thread_pool] = mock;

    secdist::RedisSettings settings;
    settings.shards = {std::string{MockSentinelServers::kRedisName}};
    settings.sentinels.reserve(std::size(sentinels));
    for (const auto& sentinel : sentinels) {
        settings.sentinels.emplace_back(kLocalhost, sentinel.GetPort());
    }

    std::vector<MockRedisServer::HandlerPtr> no_auth_handlers;
    no_auth_handlers.reserve(std::size(sentinels) + std::size(masters) + std::size(slaves));
    mock.ForEachServer([&](auto& server) {
        no_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", "FAIL"));
    });

    mock.CreateSubscribeSentinelClientAndWait(settings);

    for (auto& handler : no_auth_handlers) {
        EXPECT_FALSE(handler->WaitForFirstReply(kWaitPeriod));
    }
}

UTEST(Redis, Select) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto select_handler = server.RegisterStatusReplyHandler("SELECT", "OK");

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    auto redis = std::make_shared<
        storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings, kDbName, stats);
    redis->Connect({kLocalhost}, server.GetPort(), {}, kRedisDatabaseIndex);

    EXPECT_TRUE(select_handler->WaitForFirstReply(kSuccessTimeout));
    EXPECT_TRUE(ping_handler->WaitForFirstReply(kSuccessTimeout));
}

UTEST(Redis, SelectFail) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto select_error_handler = server.RegisterErrorReplyHandler("SELECT", "NO PASARAN");

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    auto redis = std::make_shared<
        storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings, kDbName, stats);
    redis->Connect({kLocalhost}, server.GetPort(), {}, kRedisDatabaseIndex);

    EXPECT_TRUE(select_error_handler->WaitForFirstReply(kSuccessTimeout));
    PeriodicCheck([&] { return !IsConnected(*redis); });
}

UTEST(Redis, SelectTimeout) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto sleep_period = storages::redis::kDefaultTimeoutSingle + std::chrono::milliseconds(30);
    auto select_error_handler = server.RegisterTimeoutHandler("SELECT", sleep_period);

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    auto redis = std::make_shared<
        storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings, kDbName, stats);
    redis->Connect({kLocalhost}, server.GetPort(), {}, kRedisDatabaseIndex);

    EXPECT_TRUE(select_error_handler->WaitForFirstReply(sleep_period + kSuccessTimeout));
    PeriodicCheck([&] { return !IsConnected(*redis); });
}

UTEST_MT(Redis, SlaveREADONLY, 2) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto readonly_handler = server.RegisterStatusReplyHandler("READONLY", "OK");

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    storages::redis::RedisCreationSettings redis_settings;
    redis_settings.send_readonly = true;
    storages::redis::impl::Statistics stats;
    auto redis = std::make_shared<
        storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings, kDbName, stats);
    redis->Connect({kLocalhost}, server.GetPort(), {}, kDatabaseIndex);

    EXPECT_TRUE(readonly_handler->WaitForFirstReply(kSuccessTimeout));
    PeriodicWait([&] { return IsConnected(*redis); });
}

UTEST(Redis, SlaveREADONLYFail) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto readonly_handler = server.RegisterErrorReplyHandler("READONLY", "FAIL");

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    storages::redis::RedisCreationSettings redis_settings;
    redis_settings.send_readonly = true;
    storages::redis::impl::Statistics stats;
    auto redis = std::make_shared<
        storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings, kDbName, stats);
    redis->Connect({kLocalhost}, server.GetPort(), {}, kDatabaseIndex);

    EXPECT_TRUE(readonly_handler->WaitForFirstReply(kSuccessTimeout));
    PeriodicWait([&] { return !IsConnected(*redis); });
}

UTEST(Redis, PingFail) {
    MockRedisServer server{"redis_db"};
    auto ping_error_handler = server.RegisterErrorReplyHandler("PING", "PONG");

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    auto redis = std::make_shared<
        storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings, kDbName, stats);
    redis->Connect(
        {kLocalhost},
        server.GetPort(),
        storages::redis::Credentials{"", storages::redis::Password("")},
        kDatabaseIndex
    );

    EXPECT_TRUE(ping_error_handler->WaitForFirstReply(kSuccessTimeout));
    PeriodicWait([&] { return !IsConnected(*redis); });
}

UTEST(Redis, TimedOutCommandReleasesCallback) {
    MockRedisServer server{kDbName};
    auto ping_handler = server.RegisterPingHandler();
    auto get_reply = server.RegisterPausedReplyHandler("GET", storages::redis::ReplyData::CreateStatus("OK"));
    auto set_handler = server.RegisterStatusReplyHandler("SET", "OK");

    std::atomic<bool> timeout_seen{false};
    std::atomic<bool> follow_up_ok{false};
    std::atomic<std::size_t> callback_count{0};
    engine::SingleConsumerEvent timeout_event;
    engine::SingleConsumerEvent lifetime_destroyed;
    engine::SingleConsumerEvent follow_up_received;

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    auto redis = std::make_shared<
        storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings, kDbName, stats);
    redis->Connect({kLocalhost}, server.GetPort(), {}, kDatabaseIndex);

    EXPECT_TRUE(ping_handler->WaitForFirstReply(kSuccessTimeout));
    PeriodicWait([&] { return IsConnected(*redis); });

    struct LifetimeNotifier {
        explicit LifetimeNotifier(engine::SingleConsumerEvent& destroyed)
            : destroyed(destroyed)
        {}

        ~LifetimeNotifier() { destroyed.Send(); }

        engine::SingleConsumerEvent& destroyed;
    };

    auto lifetime = std::make_shared<LifetimeNotifier>(lifetime_destroyed);
    const std::weak_ptr<LifetimeNotifier> weak_lifetime = lifetime;
    storages::redis::CommandControl command_control;
    command_control.timeout_single = std::chrono::milliseconds{100};
    auto command = storages::redis::impl::PrepareCommand(
        {"GET", "key"},
        [lifetime = std::move(lifetime),
         &timeout_seen,
         &callback_count,
         &timeout_event](const storages::redis::impl::CommandPtr&, const storages::redis::ReplyPtr& reply) {
            static_cast<void>(lifetime);
            ++callback_count;
            timeout_seen = reply->status == storages::redis::ReplyStatus::kTimeoutError;
            timeout_event.Send();
        },
        command_control
    );
    ASSERT_TRUE(redis->AsyncCommand(command));
    command.reset();

    ASSERT_TRUE(get_reply->WaitForRequest(kSuccessTimeout));
    EXPECT_TRUE(timeout_event.WaitForEventFor(kSuccessTimeout));
    EXPECT_TRUE(lifetime_destroyed.WaitForEventFor(kSuccessTimeout));
    EXPECT_TRUE(timeout_seen.load());
    EXPECT_TRUE(weak_lifetime.expired());

    get_reply->ReleaseReply();
    ASSERT_TRUE(get_reply->WaitForReplySent(kSuccessTimeout));

    storages::redis::CommandControl follow_up_control;
    follow_up_control.timeout_single = kSuccessTimeout;
    auto follow_up = storages::redis::impl::PrepareCommand(
        {"SET", "key", "ready"},
        [&follow_up_ok,
         &follow_up_received](const storages::redis::impl::CommandPtr&, const storages::redis::ReplyPtr& reply) {
            follow_up_ok = reply->status == storages::redis::ReplyStatus::kOk;
            follow_up_received.Send();
        },
        follow_up_control
    );
    ASSERT_TRUE(redis->AsyncCommand(follow_up));
    ASSERT_TRUE(follow_up_received.WaitForEventFor(kSuccessTimeout));
    EXPECT_TRUE(follow_up_ok.load());
    EXPECT_TRUE(set_handler->WaitForFirstReply(kSuccessTimeout));
    EXPECT_EQ(callback_count.load(), 1);
}

class RedisDisconnectingReplies : public ::testing::TestWithParam<const char*> {};

INSTANTIATE_UTEST_SUITE_P(
    /**/,
    RedisDisconnectingReplies,
    ::testing::Values(
        "MASTERDOWN Link with MASTER is down and "
        "slave-serve-stale-data is set to 'no'.",
        "LOADING Redis is loading the dataset in memory",
        "READONLY You can't write against a read only slave"
    )
);

UTEST_P(RedisDisconnectingReplies, X) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto get_handler = server.RegisterErrorReplyHandler("GET", GetParam());

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    const storages::redis::RedisCreationSettings redis_settings;
    storages::redis::impl::Statistics stats;
    auto redis = std::make_shared<
        storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings, kDbName, stats);
    redis->Connect(
        {kLocalhost},
        server.GetPort(),
        storages::redis::Credentials{"", storages::redis::Password("")},
        kDatabaseIndex
    );

    EXPECT_TRUE(ping_handler->WaitForFirstReply(kSuccessTimeout));
    PeriodicWait([&] { return IsConnected(*redis); });

    auto cmd = storages::redis::impl::PrepareCommand(
        {"GET", "123"},
        [](const storages::redis::impl::CommandPtr&, storages::redis::ReplyPtr) {}
    );
    redis->AsyncCommand(cmd);

    EXPECT_TRUE(get_handler->WaitForFirstReply(kSuccessTimeout));
    PeriodicWait([&] { return !IsConnected(*redis); });
}

USERVER_NAMESPACE_END
