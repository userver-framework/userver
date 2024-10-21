#include <storages/redis/client_redistest.hpp>
#include <tuple>
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
    auto response_opt = redis::MakeHedgedRedisRequest<storages::redis::RequestGet>(
        client, &storages::redis::Client::Get, cc, settings, kKey
    );
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
    auto request = redis::MakeHedgedRedisRequestAsync<storages::redis::RequestGet>(
        client, &storages::redis::Client::Get, cc, settings, kKey
    );
    auto response_opt = request.Get();
    EXPECT_TRUE(response_opt.has_value());
    EXPECT_EQ(response_opt, kValue);
}

UTEST_F(RedisClientTest, HedgedRequestBulk) {
    auto client = GetClient();
    const storages::redis::CommandControl cc;
    const auto kKey = std::string("testkey");
    const auto kValue = std::string("testvalue");
    for (size_t i = 0; i < 10; ++i) {
        std::string key = kKey + std::to_string(i);
        std::string value = kValue + std::to_string(i);
        client->Set(key, value, cc).Get();
    }

    std::vector<std::tuple<std::string>> args;
    for (size_t i = 0; i < 10; ++i) {
        args.emplace_back(kKey + std::to_string(i));
    }

    utils::hedging::HedgingSettings settings;
    auto response = redis::MakeBulkHedgedRedisRequest<storages::redis::RequestGet>(
        client, &storages::redis::Client::Get, cc, settings, args
    );
    ASSERT_EQ(response.size(), args.size());
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_EQ(response[i], kValue + std::to_string(i));
    }
}

UTEST_F(RedisClientTest, HedgedRequestBulkAsync) {
    auto client = GetClient();
    const storages::redis::CommandControl cc;
    const auto kKey = std::string("testkey");
    const auto kValue = std::string("testvalue");
    for (size_t i = 0; i < 10; ++i) {
        std::string key = kKey + std::to_string(i);
        std::string value = kValue + std::to_string(i);
        client->Set(key, value, cc).Get();
    }

    std::vector<std::tuple<std::string>> args;
    for (size_t i = 0; i < 10; ++i) {
        args.emplace_back(kKey + std::to_string(i));
    }

    utils::hedging::HedgingSettings settings;
    auto future = redis::MakeBulkHedgedRedisRequestAsync<storages::redis::RequestGet>(
        client, &storages::redis::Client::Get, cc, settings, args
    );
    auto response = future.Get();
    ASSERT_EQ(response.size(), args.size());
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_EQ(response[i], kValue + std::to_string(i));
    }
}

USERVER_NAMESPACE_END
