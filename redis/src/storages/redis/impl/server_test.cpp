#include "mock_server_test.hpp"

#include <thread>

#include <userver/storages/redis/base.hpp>

#include <storages/redis/impl/command.hpp>
#include <storages/redis/impl/secdist_redis.hpp>
#include <storages/redis/impl/sentinel.hpp>
#include <storages/redis/impl/thread_pools.hpp>
#include <userver/dynamic_config/test_helpers.hpp>

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

}  // namespace

TEST(Redis, NoPassword) {
    MockRedisServer server;
    auto ping_handler = server.RegisterPingHandler();

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    storages::redis::RedisCreationSettings redis_settings;
    auto redis = std::make_shared<storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings);
    redis->Connect({kLocalhost}, server.GetPort(), storages::redis::Password(""), kDatabaseIndex);

    EXPECT_TRUE(ping_handler->WaitForFirstReply(kSmallPeriod));
}

TEST(Redis, Auth) {
    MockRedisServer server;
    auto ping_handler = server.RegisterPingHandler();
    auto auth_handler = server.RegisterStatusReplyHandler("AUTH", "OK");

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    storages::redis::RedisCreationSettings redis_settings;
    auto redis = std::make_shared<storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings);
    redis->Connect({kLocalhost}, server.GetPort(), storages::redis::Password("password"), kDatabaseIndex);

    EXPECT_TRUE(auth_handler->WaitForFirstReply(kSmallPeriod));
    EXPECT_TRUE(ping_handler->WaitForFirstReply(kSmallPeriod));
}

TEST(Redis, AuthFail) {
    MockRedisServer server;
    auto ping_handler = server.RegisterPingHandler();
    auto auth_error_handler = server.RegisterErrorReplyHandler("AUTH", "NO PASARAN");

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    storages::redis::RedisCreationSettings redis_settings;
    auto redis = std::make_shared<storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings);
    redis->Connect({kLocalhost}, server.GetPort(), storages::redis::Password("password"), kDatabaseIndex);

    EXPECT_TRUE(auth_error_handler->WaitForFirstReply(kSmallPeriod));
    PeriodicCheck([&] { return !IsConnected(*redis); });
}

TEST(Redis, AuthTimeout) {
    MockRedisServer server;
    auto ping_handler = server.RegisterPingHandler();
    auto sleep_period = storages::redis::kDefaultTimeoutSingle + std::chrono::milliseconds(30);
    auto auth_error_handler = server.RegisterTimeoutHandler("AUTH", sleep_period);

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    storages::redis::RedisCreationSettings redis_settings;
    auto redis = std::make_shared<storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings);
    redis->Connect({kLocalhost}, server.GetPort(), storages::redis::Password("password"), kDatabaseIndex);

    EXPECT_TRUE(auth_error_handler->WaitForFirstReply(sleep_period + kSmallPeriod));
    PeriodicCheck([&] { return !IsConnected(*redis); });
}

TEST(Redis, SentinelAuth) {
    const size_t master_count = 1;
    const size_t slave_count = 2;
    const size_t sentinel_count = 3;
    const int magic_value = 46;
    const size_t redis_thread_count = 1;
    const std::string redis_name = "redis_name";

    auto init_server_array = [&](size_t size, const std::string& description, std::optional<int> magic_value_add
                             ) -> std::vector<std::unique_ptr<MockRedisServer>> {
        std::vector<std::unique_ptr<MockRedisServer>> servers;
        for (size_t i = 0; i < size; i++) {
            servers.emplace_back(std::make_unique<MockRedisServer>(description + '-' + std::to_string(i)));
            auto& server = *servers.back();
            server.RegisterPingHandler();
            if (magic_value_add) {
                server.RegisterHandlerWithConstReply("GET", *magic_value_add + i);
            }
        }
        return servers;
    };

    auto masters = init_server_array(master_count, "masters", magic_value);
    auto slaves = init_server_array(slave_count, "slaves", magic_value);
    auto sentinels = init_server_array(sentinel_count, "sentinels", magic_value);
    auto thread_pool = std::make_shared<storages::redis::impl::ThreadPools>(1, redis_thread_count);
    std::shared_ptr<storages::redis::impl::Sentinel> sentinel_client;

    std::vector<MockRedisServer::SlaveInfo> slave_infos;
    for (const auto& slave : slaves) {
        slave_infos.emplace_back(redis_name, kLocalhost, slave->GetPort());
    }

    for (auto& sentinel : sentinels) {
        sentinel->RegisterSentinelMastersHandler({{redis_name, kLocalhost, masters.at(0)->GetPort()}});
        sentinel->RegisterSentinelSlavesHandler(redis_name, slave_infos);
    }

    std::vector<MockRedisServer::HandlerPtr> auth_handlers;
    for (auto& sentinel : sentinels) {
        auth_handlers.push_back(sentinel->RegisterStatusReplyHandler("AUTH", "OK"));
    }

    secdist::RedisSettings settings;
    settings.shards = {redis_name};
    settings.sentinel_password = storages::redis::Password("pass");
    for (const auto& sentinel : sentinels) {
        settings.sentinels.emplace_back(kLocalhost, sentinel->GetPort());
    }
    sentinel_client = storages::redis::impl::Sentinel::CreateSentinel(
        thread_pool, settings, "test_shard_group_name", dynamic_config::GetDefaultSource(), "test_client_name", {""}
    );

    sentinel_client->WaitConnectedDebug(slaves.empty());

    for (auto& handler : auth_handlers) {
        EXPECT_TRUE(handler->WaitForFirstReply(kSmallPeriod));
    }

    for (const auto& sentinel : sentinels) {
        EXPECT_TRUE(sentinel->WaitForFirstPingReply(kSmallPeriod));
    }
}

TEST(Redis, Select) {
    MockRedisServer server;
    auto ping_handler = server.RegisterPingHandler();
    auto select_handler = server.RegisterStatusReplyHandler("SELECT", "OK");

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    storages::redis::RedisCreationSettings redis_settings;
    auto redis = std::make_shared<storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings);
    redis->Connect({kLocalhost}, server.GetPort(), {}, kRedisDatabaseIndex);

    EXPECT_TRUE(select_handler->WaitForFirstReply(kSmallPeriod));
    EXPECT_TRUE(ping_handler->WaitForFirstReply(kSmallPeriod));
}

TEST(Redis, SelectFail) {
    MockRedisServer server;
    auto ping_handler = server.RegisterPingHandler();
    auto select_error_handler = server.RegisterErrorReplyHandler("SELECT", "NO PASARAN");

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    storages::redis::RedisCreationSettings redis_settings;
    auto redis = std::make_shared<storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings);
    redis->Connect({kLocalhost}, server.GetPort(), {}, kRedisDatabaseIndex);

    EXPECT_TRUE(select_error_handler->WaitForFirstReply(kSmallPeriod));
    PeriodicCheck([&] { return !IsConnected(*redis); });
}

TEST(Redis, SelectTimeout) {
    MockRedisServer server;
    auto ping_handler = server.RegisterPingHandler();
    auto sleep_period = storages::redis::kDefaultTimeoutSingle + std::chrono::milliseconds(30);
    auto select_error_handler = server.RegisterTimeoutHandler("SELECT", sleep_period);

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    storages::redis::RedisCreationSettings redis_settings;
    auto redis = std::make_shared<storages::redis::impl::Redis>(pool->GetRedisThreadPool(), redis_settings);
    redis->Connect({kLocalhost}, server.GetPort(), {}, kRedisDatabaseIndex);

    EXPECT_TRUE(select_error_handler->WaitForFirstReply(sleep_period + kSmallPeriod));
    PeriodicCheck([&] { return !IsConnected(*redis); });
}

TEST(Redis, SlaveREADONLY) {
    MockRedisServer server;
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
    MockRedisServer server;
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
    MockRedisServer server;
    auto ping_error_handler = server.RegisterErrorReplyHandler("PING", "PONG");

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    storages::redis::RedisCreationSettings redis_settings;
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
    MockRedisServer server;
    auto ping_handler = server.RegisterPingHandler();
    auto get_handler = server.RegisterErrorReplyHandler("GET", GetParam());

    auto pool = std::make_shared<storages::redis::impl::ThreadPools>(1, 1);
    storages::redis::RedisCreationSettings redis_settings;
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
