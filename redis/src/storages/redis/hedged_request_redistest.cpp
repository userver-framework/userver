#include <storages/redis/client_redistest.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/engine/wait_any.hpp>
#include <userver/storages/redis/hedged_request.hpp>

USERVER_NAMESPACE_BEGIN

UTEST_F(RedisClientTest, HedgedRequest) {
  auto client = GetClient();
  const storages::redis::CommandControl cc;
  const auto kKey = std::string("testkey");
  const auto kValue = std::string("testvalue");
  client->Set(kKey, kValue, cc).Get();

  utils::hedging::HedgingSettings settings;
  auto response_opt =
      redis::MakeHedgedRedisRequest<storages::redis::RequestGet>(
          client, &storages::redis::Client::Get, cc, settings, kKey);
  EXPECT_TRUE(response_opt.has_value());
  EXPECT_EQ(response_opt, kValue);
}

UTEST_F(RedisClientTest, HedgedRequestAsync) {
  auto client = GetClient();
  const storages::redis::CommandControl cc;
  const auto kKey = std::string("testkey");
  const auto kValue = std::string("testvalue");
  client->Set(kKey, kValue, cc).Get();

  utils::hedging::HedgingSettings settings;
  auto request =
      redis::MakeHedgedRedisRequestAsync<storages::redis::RequestGet>(
          client, &storages::redis::Client::Get, cc, settings, kKey);
  auto response_opt = request.Get();
  EXPECT_TRUE(response_opt.has_value());
  EXPECT_EQ(response_opt, kValue);
}

USERVER_NAMESPACE_END
