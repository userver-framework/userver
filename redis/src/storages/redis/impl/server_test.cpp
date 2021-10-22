#include <userver/storages/redis/impl/secdist_redis.hpp>
#include <userver/storages/redis/impl/sentinel.hpp>
#include <userver/storages/redis/impl/thread_pools.hpp>
#include "mock_server_test.hpp"

USERVER_NAMESPACE_BEGIN

namespace {
// 100ms should be enough, but valgrind is too slow
const auto kSmallPeriod = std::chrono::milliseconds(500);
const auto kWaitPeriod = std::chrono::milliseconds(10);
const auto kWaitRetries = 100;

const std::string kLocalhost = "127.0.0.1";
}  // namespace

#define EXPECT_FOR(count, duration, expect)  \
  do {                                       \
    for (int i = 0; i < (count); i++) {      \
      expect;                                \
      std::this_thread::sleep_for(duration); \
    }                                        \
  } while (false)

#define EXPECT_EQ_TIMEOUT(count, duration, a, b) \
  do {                                           \
    for (int i = 0; i < (count); i++) {          \
      if ((a) == (b)) break;                     \
      std::this_thread::sleep_for(duration);     \
    }                                            \
    EXPECT_EQ((a), (b));                         \
  } while (false)

#define EXPECT_NE_TIMEOUT(count, duration, a, b) \
  do {                                           \
    for (int i = 0; i < (count); i++) {          \
      if ((a) != (b)) break;                     \
      std::this_thread::sleep_for(duration);     \
    }                                            \
    EXPECT_NE((a), (b));                         \
  } while (false)

TEST(Redis, NoPassword) {
  MockRedisServer server;
  auto ping_handler = server.RegisterPingHandler();

  auto pool = std::make_shared<redis::ThreadPools>(1, 1);
  auto redis =
      std::make_shared<redis::Redis>(pool->GetRedisThreadPool(), false);
  redis->Connect(kLocalhost, server.GetPort(), redis::Password(""));

  EXPECT_TRUE(ping_handler->WaitForFirstReply(kSmallPeriod));
}

TEST(Redis, Auth) {
  MockRedisServer server;
  auto ping_handler = server.RegisterPingHandler();
  auto auth_handler = server.RegisterStatusReplyHandler("AUTH", "OK");

  auto pool = std::make_shared<redis::ThreadPools>(1, 1);
  auto redis =
      std::make_shared<redis::Redis>(pool->GetRedisThreadPool(), false);
  redis->Connect(kLocalhost, server.GetPort(), redis::Password("password"));

  EXPECT_TRUE(auth_handler->WaitForFirstReply(kSmallPeriod));
  EXPECT_TRUE(ping_handler->WaitForFirstReply(kSmallPeriod));
}

TEST(Redis, AuthFail) {
  MockRedisServer server;
  auto ping_handler = server.RegisterPingHandler();
  auto auth_error_handler =
      server.RegisterErrorReplyHandler("AUTH", "NO PASARAN");

  auto pool = std::make_shared<redis::ThreadPools>(1, 1);
  auto redis =
      std::make_shared<redis::Redis>(pool->GetRedisThreadPool(), false);
  redis->Connect(kLocalhost, server.GetPort(), redis::Password("password"));

  EXPECT_TRUE(auth_error_handler->WaitForFirstReply(kSmallPeriod));
  EXPECT_FOR(10, kWaitPeriod,
             EXPECT_NE(redis->GetState(), redis::Redis::State::kConnected));
}

TEST(Redis, AuthTimeout) {
  MockRedisServer server;
  auto ping_handler = server.RegisterPingHandler();
  auto sleep_period = redis::command_control_init.timeout_single +
                      std::chrono::milliseconds(30);
  auto auth_error_handler = server.RegisterTimeoutHandler("AUTH", sleep_period);

  auto pool = std::make_shared<redis::ThreadPools>(1, 1);
  auto redis =
      std::make_shared<redis::Redis>(pool->GetRedisThreadPool(), false);
  redis->Connect(kLocalhost, server.GetPort(), redis::Password("password"));

  EXPECT_TRUE(
      auth_error_handler->WaitForFirstReply(sleep_period + kSmallPeriod));
  EXPECT_FOR(10, kWaitPeriod,
             EXPECT_NE(redis->GetState(), redis::Redis::State::kConnected));
}

TEST(Redis, SlaveREADONLY) {
  MockRedisServer server;
  auto ping_handler = server.RegisterPingHandler();
  auto readonly_handler = server.RegisterStatusReplyHandler("READONLY", "OK");

  auto pool = std::make_shared<redis::ThreadPools>(1, 1);
  auto redis = std::make_shared<redis::Redis>(pool->GetRedisThreadPool(), true);
  redis->Connect(kLocalhost, server.GetPort(), {});

  EXPECT_TRUE(readonly_handler->WaitForFirstReply(kSmallPeriod));
  EXPECT_EQ_TIMEOUT(kWaitRetries, kWaitPeriod, redis->GetState(),
                    redis::Redis::State::kConnected);
}

TEST(Redis, SlaveREADONLYFail) {
  MockRedisServer server;
  auto ping_handler = server.RegisterPingHandler();
  auto readonly_handler = server.RegisterErrorReplyHandler("READONLY", "FAIL");

  auto pool = std::make_shared<redis::ThreadPools>(1, 1);
  auto redis = std::make_shared<redis::Redis>(pool->GetRedisThreadPool(), true);
  redis->Connect(kLocalhost, server.GetPort(), {});

  EXPECT_TRUE(readonly_handler->WaitForFirstReply(kSmallPeriod));
  EXPECT_NE_TIMEOUT(kWaitRetries, kWaitPeriod, redis->GetState(),
                    redis::Redis::State::kConnected);
}

TEST(Redis, PingFail) {
  MockRedisServer server;
  auto ping_error_handler = server.RegisterErrorReplyHandler("PING", "PONG");

  auto pool = std::make_shared<redis::ThreadPools>(1, 1);
  auto redis =
      std::make_shared<redis::Redis>(pool->GetRedisThreadPool(), false);
  redis->Connect(kLocalhost, server.GetPort(), redis::Password(""));

  EXPECT_TRUE(ping_error_handler->WaitForFirstReply(kSmallPeriod));
  EXPECT_NE_TIMEOUT(kWaitRetries, kWaitPeriod, redis->GetState(),
                    redis::Redis::State::kConnected);
}

class RedisDisconnectingReplies : public ::testing::TestWithParam<const char*> {
};

INSTANTIATE_TEST_SUITE_P(
    /**/, RedisDisconnectingReplies,
    ::testing::Values("MASTERDOWN Link with MASTER is down and "
                      "slave-serve-stale-data is set to 'no'.",
                      "LOADING Redis is loading the dataset in memory",
                      "READONLY You can't write against a read only slave"));

TEST_P(RedisDisconnectingReplies, X) {
  MockRedisServer server;
  auto ping_handler = server.RegisterPingHandler();
  auto get_handler = server.RegisterErrorReplyHandler("GET", GetParam());

  auto pool = std::make_shared<redis::ThreadPools>(1, 1);
  auto redis =
      std::make_shared<redis::Redis>(pool->GetRedisThreadPool(), false);
  redis->Connect(kLocalhost, server.GetPort(), redis::Password(""));

  EXPECT_TRUE(ping_handler->WaitForFirstReply(kSmallPeriod));
  EXPECT_EQ_TIMEOUT(kWaitRetries, kWaitPeriod, redis->GetState(),
                    redis::Redis::State::kConnected);

  auto cmd = redis::PrepareCommand(
      {"GET", "123"}, [](const redis::CommandPtr&, redis::ReplyPtr) {});
  redis->AsyncCommand(cmd);

  EXPECT_TRUE(get_handler->WaitForFirstReply(kSmallPeriod));
  EXPECT_NE_TIMEOUT(kWaitRetries, kWaitPeriod, redis->GetState(),
                    redis::Redis::State::kConnected);
}

USERVER_NAMESPACE_END
