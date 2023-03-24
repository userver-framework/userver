#pragma once

#include <regex>
#include <string>

#include <userver/utest/utest.hpp>

#include <userver/storages/redis/impl/thread_pools.hpp>

#include <storages/redis/client_impl.hpp>
#include <storages/redis/impl/sentinel.hpp>
#include <storages/redis/util_redistest.hpp>

USERVER_NAMESPACE_BEGIN

class RedisClientTest : public ::testing::Test {
 public:
  struct Version {
    int major{};
    int minor{};
    int patch{};
  };

  void SetUp() override {
    auto thread_pools = std::make_shared<redis::ThreadPools>(
        redis::kDefaultSentinelThreadPoolSize,
        redis::kDefaultRedisThreadPoolSize);
    auto sentinel = redis::Sentinel::CreateSentinel(
        std::move(thread_pools), GetTestsuiteRedisSettings(), "none", "pub",
        redis::KeyShardFactory{""});
    sentinel->WaitConnectedDebug();

    sentinel
        ->MakeRequest({"flushdb"}, "none", true, redis::kDefaultCommandControl)
        .Get();

    auto info_reply = sentinel
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

    client_ =
        std::make_shared<storages::redis::ClientImpl>(std::move(sentinel));
  }

  storages::redis::ClientPtr GetClient() { return client_; }

  bool CheckVersion(Version since) {
    return std::tie(since.major, since.minor, since.patch) <=
           std::tie(version_.major, version_.minor, version_.patch);
  }

  static std::string SkipMsgByVersion(std::string_view command,
                                      Version version) {
    return fmt::format("{} command available since {}.{}.{}", command,
                       version.major, version.minor, version.patch);
  }

 private:
  Version version_{};
  storages::redis::ClientPtr client_{};

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

USERVER_NAMESPACE_END
