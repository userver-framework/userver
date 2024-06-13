#pragma once

#include <userver/utest/utest.hpp>

#include <userver/utils/regex.hpp>

#include <storages/redis/utest/impl/redis_connection_state.hpp>

USERVER_NAMESPACE_BEGIN

// This fixture is used for both Client and SubscribeClient tests, because
// Publish() method is part of Client - not SubscribeClient. And without
// Publish() there is no way to write proper unit tests for subscribing
template <class BaseRedisConnectionState>
class BaseRedisClientTestEx  // NOLINT(fuchsia-multiple-inheritance)
    : public ::testing::Test,
      public BaseRedisConnectionState {
 public:
  struct Version {
    int major{};
    int minor{};
    int patch{};
  };

  void SetUp() override {
    auto info_reply = this->GetSentinel()
                          ->MakeRequest({"info", "server"}, "none", false)
                          .Get();
    ASSERT_TRUE(info_reply->IsOk());
    ASSERT_TRUE(info_reply->data.IsString());
    const auto info = info_reply->data.GetString();

    utils::regex redis_version_regex(R"(redis_version:(\d+.\d+.\d+))");
    utils::smatch redis_version_matches;
    ASSERT_TRUE(
        utils::regex_search(info, redis_version_matches, redis_version_regex));
    version_ = MakeVersion(redis_version_matches[1]);
  }

  bool CheckVersion(Version since) const {
    return std::tie(since.major, since.minor, since.patch) <=
           std::tie(version_.major, version_.minor, version_.patch);
  }

  static std::string SkipMsgByVersion(std::string_view command,
                                      Version version) {
    return fmt::format("{} command available since {}.{}.{}", command,
                       version.major, version.minor, version.patch);
  }

 private:
  static Version MakeVersion(std::string from) {
    utils::regex rgx(R"((\d+).(\d+).(\d+))");
    utils::smatch matches;
    const auto result = utils::regex_search(from, matches, rgx);
    EXPECT_TRUE(result);
    if (!result) return {};
    return {std::stoi(matches[1]), std::stoi(matches[2]),
            std::stoi(matches[3])};
  }

  Version version_;
};

using BaseRedisClientTest =
    BaseRedisClientTestEx<storages::redis::utest::impl::RedisConnectionState>;

using BaseRedisClusterClientTest = BaseRedisClientTestEx<
    storages::redis::utest::impl::RedisClusterConnectionState>;

USERVER_NAMESPACE_END
