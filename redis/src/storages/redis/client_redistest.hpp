#pragma once

#include <memory>
#include <regex>
#include <string>

#include <userver/utest/utest.hpp>

#include <userver/storages/redis/impl/thread_pools.hpp>

#include <storages/redis/client_impl.hpp>
#include <storages/redis/dynamic_config.hpp>
#include <storages/redis/impl/sentinel.hpp>
#include <storages/redis/impl/subscribe_sentinel.hpp>
#include <storages/redis/subscribe_client_impl.hpp>
#include <storages/redis/util_redistest.hpp>
#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/dynamic_config/test_helpers.hpp>

USERVER_NAMESPACE_BEGIN

// This fixture is used for both Client and SubscribeClient tests, because
// Publish() method is part of Client - not SubscribeClient. And without
// Publish() there is no way to write proper unit tests for subscribing
class RedisClientTest : public ::testing::Test {
 public:
  struct Version {
    int major{};
    int minor{};
    int patch{};
  };

  static void SetUpTestSuite() {
    thread_pools_ = std::make_shared<redis::ThreadPools>(
        redis::kDefaultSentinelThreadPoolSize,
        redis::kDefaultRedisThreadPoolSize);
    sentinel_ = redis::Sentinel::CreateSentinel(
        thread_pools_, GetTestsuiteRedisSettings(), "none",
        dynamic_config::GetDefaultSource(), "pub", redis::KeyShardFactory{""});
    sentinel_->WaitConnectedDebug();
    subscribe_sentinel_ = redis::SubscribeSentinel::Create(
        thread_pools_, GetTestsuiteRedisSettings(), "none",
        dynamic_config::GetDefaultSource(), "pub", false, {}, nullptr);
    subscribe_sentinel_->WaitConnectedDebug();

    auto info_reply = sentinel_
                          ->MakeRequest({"info", "server"}, "none", false,
                                        redis::kDefaultCommandControl)
                          .Get();
    ASSERT_TRUE(info_reply->IsOk());
    ASSERT_TRUE(info_reply->data.IsString());
    const auto info = info_reply->data.GetString();

    std::regex redis_version_regex(R"(redis_version:(\d+.\d+.\d+))");
    std::smatch redis_version_matches;
    ASSERT_TRUE(
        std::regex_search(info, redis_version_matches, redis_version_regex));
    version_ = MakeVersion(redis_version_matches[1]);
  }

  static void TearDownTestSuite() {
    subscribe_sentinel_.reset();
    sentinel_.reset();
    thread_pools_.reset();
    version_ = {};
  }

  void SetUp() override {
    sentinel_
        ->MakeRequest({"flushdb"}, "none", true, redis::kDefaultCommandControl)
        .Get();

    client_ = std::make_shared<storages::redis::ClientImpl>(sentinel_);

    /* Subscribe client */
    subscribe_client_ = std::make_shared<storages::redis::SubscribeClientImpl>(
        subscribe_sentinel_);
  }

  static std::shared_ptr<redis::Sentinel> GetSentinel() { return sentinel_; }

  storages::redis::ClientPtr GetClient() { return client_; }
  std::shared_ptr<storages::redis::SubscribeClient> GetSubscribeClient() {
    return subscribe_client_;
  }

  static bool CheckVersion(Version since) {
    return std::tie(since.major, since.minor, since.patch) <=
           std::tie(version_.major, version_.minor, version_.patch);
  }

  static std::string SkipMsgByVersion(std::string_view command,
                                      Version version) {
    return fmt::format("{} command available since {}.{}.{}", command,
                       version.major, version.minor, version.patch);
  }

 private:
  static std::shared_ptr<redis::ThreadPools> thread_pools_;
  static std::shared_ptr<redis::Sentinel> sentinel_;
  static std::shared_ptr<redis::SubscribeSentinel> subscribe_sentinel_;
  static Version version_;
  storages::redis::ClientPtr client_{};
  std::shared_ptr<storages::redis::SubscribeClient> subscribe_client_{};

  static Version MakeVersion(std::string from) {
    std::regex rgx(R"((\d+).(\d+).(\d+))");
    std::smatch matches;
    const auto result = std::regex_search(from, matches, rgx);
    EXPECT_TRUE(result);
    if (!result) return {};
    return {std::stoi(matches[1]), std::stoi(matches[2]),
            std::stoi(matches[3])};
  }
};

inline std::shared_ptr<redis::ThreadPools> RedisClientTest::thread_pools_;
inline std::shared_ptr<redis::Sentinel> RedisClientTest::sentinel_;
inline std::shared_ptr<redis::SubscribeSentinel>
    RedisClientTest::subscribe_sentinel_;
inline RedisClientTest::Version RedisClientTest::version_;

USERVER_NAMESPACE_END
