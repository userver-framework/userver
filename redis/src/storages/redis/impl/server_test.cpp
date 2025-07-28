#include "mock_server_test.hpp"

#include <thread>

#include <userver/storages/redis/base.hpp>

#include <storages/redis/impl/command.hpp>
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

// 100ms should be enough, but valgrind is too slow
constexpr std::chrono::milliseconds kSmallPeriod{500};
constexpr std::chrono::milliseconds kWaitPeriod{10};
constexpr auto kWaitRetries = 100;
constexpr auto kCheckCount = 10;
constexpr auto kRedisDatabaseIndex = 46;
constexpr std::size_t kDatabaseIndex = 0;

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
        if (predicate()) break;
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
    std::shared_ptr<storages::redis::impl::ThreadPools> thread_pool =
        std::make_shared<storages::redis::impl::ThreadPools>(1, kRedisThreadCount);
};

}  // namespace

TEST(Redis, NoPassword) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    const storages::redis::RedisCreationSettings redis_settings;
    auto redis = std::make_shared<storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings);
    redis->Connect({kLocalhost}, server.GetPort(), storages::redis::Password(""), kDatabaseIndex);

    EXPECT_TRUE(ping_handler->WaitForFirstReply(kSmallPeriod));
}

TEST(Redis, Auth) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto auth_handler = server.RegisterStatusReplyHandler("AUTH", "OK");

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    const storages::redis::RedisCreationSettings redis_settings;
    auto redis = std::make_shared<storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings);
    redis->Connect({kLocalhost}, server.GetPort(), storages::redis::Password("password"), kDatabaseIndex);

    EXPECT_TRUE(auth_handler->WaitForFirstReply(kSmallPeriod));
    EXPECT_TRUE(ping_handler->WaitForFirstReply(kSmallPeriod));
}

TEST(Redis, AuthFail) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto auth_error_handler = server.RegisterErrorReplyHandler("AUTH", "NO PASARAN");

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    const storages::redis::RedisCreationSettings redis_settings;
    auto redis = std::make_shared<storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings);
    redis->Connect({kLocalhost}, server.GetPort(), storages::redis::Password("password"), kDatabaseIndex);

    EXPECT_TRUE(auth_error_handler->WaitForFirstReply(kSmallPeriod));
    PeriodicCheck([&] { return !IsConnected(*redis); });
}

TEST(Redis, AuthTimeout) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto sleep_period = storages::redis::kDefaultTimeoutSingle + std::chrono::milliseconds(30);
    auto auth_error_handler = server.RegisterTimeoutHandler("AUTH", sleep_period);

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    const storages::redis::RedisCreationSettings redis_settings;
    auto redis = std::make_shared<storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings);
    redis->Connect({kLocalhost}, server.GetPort(), storages::redis::Password("password"), kDatabaseIndex);

    EXPECT_TRUE(auth_error_handler->WaitForFirstReply(sleep_period + kSmallPeriod));
    PeriodicCheck([&] { return !IsConnected(*redis); });
}

UTEST(Redis, SentinelAuth) {
    MockSentinelServers mock;
    mock.RegisterSentinelMastersSlaves();
    mock.ForEachServer([](auto& server) { server.RegisterPingHandler(); });
    auto& [masters, slaves, sentinels, thread_pool] = mock;

    secdist::RedisSettings settings;
    settings.shards = {std::string{MockSentinelServers::kRedisName}};
    settings.sentinel_password = storages::redis::Password("pass");
    settings.sentinels.reserve(std::size(sentinels));
    for (const auto& sentinel : sentinels) {
        settings.sentinels.emplace_back(kLocalhost, sentinel.GetPort());
    }

    std::vector<MockRedisServer::HandlerPtr> auth_handlers;
    auth_handlers.reserve(std::size(sentinels));
    for (auto& sentinel : sentinels) {
        auth_handlers.push_back(sentinel.RegisterStatusReplyHandler("AUTH", "OK"));
    }
    std::vector<MockRedisServer::HandlerPtr> no_auth_handlers;
    no_auth_handlers.reserve(std::size(masters) + std::size(slaves));
    for (auto& server : masters) {
        no_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", "FAIL"));
    }
    for (auto& server : slaves) {
        no_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", "FAIL"));
    }

    auto sentinel_client = storages::redis::impl::Sentinel::CreateSentinel(
        thread_pool, settings, "test_shard_group_name", dynamic_config::GetDefaultSource(), "test_client_name", {""}
    );
    sentinel_client->WaitConnectedDebug(std::empty(slaves));

    for (auto& handler : auth_handlers) {
        EXPECT_TRUE(handler->WaitForFirstReply(kSmallPeriod));
    }

    for (auto& handler : no_auth_handlers) {
        EXPECT_FALSE(handler->WaitForFirstReply(kWaitPeriod));
    }

    for (const auto& sentinel : sentinels) {
        EXPECT_TRUE(sentinel.WaitForFirstPingReply(kSmallPeriod));
    }
}

// TODO: TAXICOMMON-10834. Looks like AUTH to sentinel is not sent in case of SUBSCRIBE
UTEST(Redis, DISABLED_SentinelAuthSubscribe) {
    MockSentinelServers mock;
    mock.RegisterSentinelMastersSlaves();
    mock.ForEachServer([](auto& server) { server.RegisterPingHandler(); });
    auto& [masters, slaves, sentinels, thread_pool] = mock;
    // Sentinels do NOT receive SUBSCRIBE
    std::vector<MockRedisServer::HandlerPtr> subscribe_handlers;
    for (auto& server : masters) {
        subscribe_handlers.push_back(server.RegisterHandlerWithConstReply("SUBSCRIBE", 1));
    }
    for (auto& server : slaves) {
        subscribe_handlers.push_back(server.RegisterHandlerWithConstReply("SUBSCRIBE", 1));
    }

    secdist::RedisSettings settings;
    settings.shards = {std::string{MockSentinelServers::kRedisName}};
    settings.sentinel_password = storages::redis::Password("pass");
    settings.sentinels.reserve(std::size(sentinels));
    for (const auto& sentinel : sentinels) {
        settings.sentinels.emplace_back(kLocalhost, sentinel.GetPort());
    }

    std::vector<MockRedisServer::HandlerPtr> auth_handlers;
    auth_handlers.reserve(std::size(sentinels));
    for (auto& sentinel : sentinels) {
        auth_handlers.push_back(sentinel.RegisterStatusReplyHandler("AUTH", "OK"));
    }
    std::vector<MockRedisServer::HandlerPtr> no_auth_handlers;
    no_auth_handlers.reserve(std::size(masters) + std::size(slaves));
    for (auto& server : masters) {
        no_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", "FAIL"));
    }
    for (auto& server : slaves) {
        no_auth_handlers.push_back(server.RegisterStatusReplyHandler("AUTH", "FAIL"));
    }

    storages::redis::CommandControl cc{};
    testsuite::RedisControl redis_control{};
    auto subscribe_sentinel = storages::redis::impl::SubscribeSentinel::Create(
        thread_pool,
        settings,
        "test_shard_group_name",
        dynamic_config::GetDefaultSource(),
        "test_client_name",
        {""},
        cc,
        redis_control
    );
    subscribe_sentinel->WaitConnectedDebug(std::empty(slaves));
    std::shared_ptr<storages::redis::SubscribeClient> client =
        std::make_shared<storages::redis::SubscribeClientImpl>(std::move(subscribe_sentinel));

    storages::redis::SubscriptionToken::OnMessageCb callback = [](const std::string& channel,
                                                                  const std::string& message) {
        EXPECT_TRUE(false) << "Should not be called. Channel = " << channel << ", message = " << message;
    };
    auto subscription = client->Subscribe("channel_name", std::move(callback));

    for (auto& handler : subscribe_handlers) {
        EXPECT_TRUE(handler->WaitForFirstReply(utest::kMaxTestWaitTime));
    }

    for (auto& handler : auth_handlers) {
        EXPECT_TRUE(handler->WaitForFirstReply(kSmallPeriod));
    }

    for (auto& handler : no_auth_handlers) {
        EXPECT_FALSE(handler->WaitForFirstReply(kWaitPeriod));
    }
}

TEST(Redis, Select) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto select_handler = server.RegisterStatusReplyHandler("SELECT", "OK");

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    const storages::redis::RedisCreationSettings redis_settings;
    auto redis = std::make_shared<storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings);
    redis->Connect({kLocalhost}, server.GetPort(), {}, kRedisDatabaseIndex);

    EXPECT_TRUE(select_handler->WaitForFirstReply(kSmallPeriod));
    EXPECT_TRUE(ping_handler->WaitForFirstReply(kSmallPeriod));
}

TEST(Redis, SelectFail) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto select_error_handler = server.RegisterErrorReplyHandler("SELECT", "NO PASARAN");

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    const storages::redis::RedisCreationSettings redis_settings;
    auto redis = std::make_shared<storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings);
    redis->Connect({kLocalhost}, server.GetPort(), {}, kRedisDatabaseIndex);

    EXPECT_TRUE(select_error_handler->WaitForFirstReply(kSmallPeriod));
    PeriodicCheck([&] { return !IsConnected(*redis); });
}

TEST(Redis, SelectTimeout) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto sleep_period = storages::redis::kDefaultTimeoutSingle + std::chrono::milliseconds(30);
    auto select_error_handler = server.RegisterTimeoutHandler("SELECT", sleep_period);

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    const storages::redis::RedisCreationSettings redis_settings;
    auto redis = std::make_shared<storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings);
    redis->Connect({kLocalhost}, server.GetPort(), {}, kRedisDatabaseIndex);

    EXPECT_TRUE(select_error_handler->WaitForFirstReply(sleep_period + kSmallPeriod));
    PeriodicCheck([&] { return !IsConnected(*redis); });
}

TEST(Redis, SlaveREADONLY) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto readonly_handler = server.RegisterStatusReplyHandler("READONLY", "OK");

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    storages::redis::RedisCreationSettings redis_settings;
    redis_settings.send_readonly = true;
    auto redis = std::make_shared<storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings);
    redis->Connect({kLocalhost}, server.GetPort(), {}, kDatabaseIndex);

    EXPECT_TRUE(readonly_handler->WaitForFirstReply(kSmallPeriod));
    PeriodicWait([&] { return IsConnected(*redis); });
}

TEST(Redis, SlaveREADONLYFail) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto readonly_handler = server.RegisterErrorReplyHandler("READONLY", "FAIL");

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    storages::redis::RedisCreationSettings redis_settings;
    redis_settings.send_readonly = true;
    auto redis = std::make_shared<storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings);
    redis->Connect({kLocalhost}, server.GetPort(), {}, kDatabaseIndex);

    EXPECT_TRUE(readonly_handler->WaitForFirstReply(kSmallPeriod));
    PeriodicWait([&] { return !IsConnected(*redis); });
}

TEST(Redis, PingFail) {
    MockRedisServer server{"redis_db"};
    auto ping_error_handler = server.RegisterErrorReplyHandler("PING", "PONG");

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    const storages::redis::RedisCreationSettings redis_settings;
    auto redis = std::make_shared<storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings);
    redis->Connect({kLocalhost}, server.GetPort(), storages::redis::Password(""), kDatabaseIndex);

    EXPECT_TRUE(ping_error_handler->WaitForFirstReply(kSmallPeriod));
    PeriodicWait([&] { return !IsConnected(*redis); });
}

class RedisDisconnectingReplies : public ::testing::TestWithParam<const char*> {};

INSTANTIATE_TEST_SUITE_P(
    /**/,
    RedisDisconnectingReplies,
    ::testing::Values(
        "MASTERDOWN Link with MASTER is down and "
        "slave-serve-stale-data is set to 'no'.",
        "LOADING Redis is loading the dataset in memory",
        "READONLY You can't write against a read only slave"
    )
);

TEST_P(RedisDisconnectingReplies, X) {
    MockRedisServer server{"redis_db"};
    auto ping_handler = server.RegisterPingHandler();
    auto get_handler = server.RegisterErrorReplyHandler("GET", GetParam());

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    const storages::redis::RedisCreationSettings redis_settings;
    auto redis = std::make_shared<storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings);
    redis->Connect({kLocalhost}, server.GetPort(), storages::redis::Password(""), kDatabaseIndex);

    EXPECT_TRUE(ping_handler->WaitForFirstReply(kSmallPeriod));
    PeriodicWait([&] { return IsConnected(*redis); });

    auto cmd = storages::redis::impl::PrepareCommand(
        {"GET", "123"}, [](const storages::redis::impl::CommandPtr&, storages::redis::ReplyPtr) {}
    );
    redis->AsyncCommand(cmd);

    EXPECT_TRUE(get_handler->WaitForFirstReply(kSmallPeriod));
    PeriodicWait([&] { return !IsConnected(*redis); });
}

USERVER_NAMESPACE_END
