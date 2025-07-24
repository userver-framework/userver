#include "mock_server_test.hpp"

#include <thread>

#include <userver/storages/redis/base.hpp>

#include <storages/redis/impl/command.hpp>
#include <storages/redis/impl/secdist_redis.hpp>
#include <storages/redis/impl/sentinel.hpp>
#include <storages/redis/impl/thread_pools.hpp>

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
