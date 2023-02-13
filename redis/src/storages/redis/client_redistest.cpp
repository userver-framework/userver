#include <userver/utest/utest.hpp>

#include <memory>
#include <regex>
#include <string>
#include <tuple>

#include <userver/engine/task/cancel.hpp>

#include <storages/redis/client_impl.hpp>
#include <storages/redis/util_redistest.hpp>
#include <userver/storages/redis/impl/sentinel.hpp>
#include <userver/storages/redis/impl/thread_pools.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class RedisClient : public ::testing::Test {
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
        ->MakeRequest({"FLUSHDB"}, "none", true, redis::kDefaultCommandControl)
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

  std::shared_ptr<storages::redis::Client> GetClient() { return client_; }

  bool CheckVersion(Version since) {
    return std::tie(since.major, since.minor, since.patch) <=
           std::tie(version_.major, version_.minor, version_.patch);
  }

 private:
  Version version_;
  std::shared_ptr<storages::redis::Client> client_;

  static Version MakeVersion(std::string from) {
    Version version;
    std::regex rgx(R"((\d+).(\d+).(\d+))");
    std::smatch matches;
    const auto result = std::regex_search(from, matches, rgx);
    EXPECT_TRUE(result);
    if (!result) return {};
    version.major = stoi(matches[1]);
    version.minor = stoi(matches[2]);
    version.patch = stoi(matches[3]);
    return version;
  }
};

/// [Sample Redis Client usage]
void RedisClientSampleUsage(storages::redis::Client& client) {
  client.Rpush("sample_list", "a", {}).Get();
  client.Rpush("sample_list", "b", {}).Get();
  const auto length = client.Llen("sample_list", {}).Get();

  EXPECT_EQ(length, 2);
}
/// [Sample Redis Client usage]

/// [Sample Redis Cancel request]
std::optional<std::string> RedisClientCancelRequest(
    storages::redis::Client& client) {
  auto result = client.Get("foo", {});

  engine::current_task::GetCancellationToken().RequestCancel();

  // Throws redis::RequestCancelledException if Redis was not
  // fast enough to answer
  return result.Get();
}
/// [Sample Redis Cancel request]

}  // namespace

UTEST_F(RedisClient, Sample) { RedisClientSampleUsage(*GetClient()); }

UTEST_F(RedisClient, CancelRequest) {
  try {
    EXPECT_FALSE(RedisClientCancelRequest(*GetClient()));
  } catch (const redis::RequestCancelledException&) {
  }
}

UTEST_F(RedisClient, Lrem) {
  auto client = GetClient();
  EXPECT_EQ(client->Rpush("testlist", "a", {}).Get(), 1);
  EXPECT_EQ(client->Rpush("testlist", "b", {}).Get(), 2);
  EXPECT_EQ(client->Rpush("testlist", "a", {}).Get(), 3);
  EXPECT_EQ(client->Rpush("testlist", "b", {}).Get(), 4);
  EXPECT_EQ(client->Rpush("testlist", "b", {}).Get(), 5);
  EXPECT_EQ(client->Llen("testlist", {}).Get(), 5);

  EXPECT_EQ(client->Lrem("testlist", 1, "b", {}).Get(), 1);
  EXPECT_EQ(client->Llen("testlist", {}).Get(), 4);
  EXPECT_EQ(client->Lrem("testlist", -2, "b", {}).Get(), 2);
  EXPECT_EQ(client->Llen("testlist", {}).Get(), 2);
  EXPECT_EQ(client->Lrem("testlist", 0, "c", {}).Get(), 0);
  EXPECT_EQ(client->Llen("testlist", {}).Get(), 2);
  EXPECT_EQ(client->Lrem("testlist", 0, "a", {}).Get(), 2);
  EXPECT_EQ(client->Llen("testlist", {}).Get(), 0);
}

UTEST_F(RedisClient, Lpushx) {
  auto client = GetClient();
  // Missing array - does nothing
  EXPECT_EQ(client->Lpushx("pushx_testlist", "a", {}).Get(), 0);
  EXPECT_EQ(client->Rpushx("pushx_testlist", "a", {}).Get(), 0);
  // Init list
  EXPECT_EQ(client->Lpush("pushx_testlist", "a", {}).Get(), 1);
  // List exists - appends values
  EXPECT_EQ(client->Lpushx("pushx_testlist", "a", {}).Get(), 2);
  EXPECT_EQ(client->Rpushx("pushx_testlist", "a", {}).Get(), 3);
}

UTEST_F(RedisClient, SetGet) {
  auto client = GetClient();
  UEXPECT_NO_THROW(client->Set("key0", "foo", {}).Get());
  EXPECT_EQ(client->Get("key0", {}).Get(), "foo");
}

UTEST_F(RedisClient, Mget) {
  auto client = GetClient();
  client->Set("key0", "foo", {}).Get();
  client->Set("key1", "bar", {}).Get();

  std::vector<std::string> keys;
  keys.reserve(100);
  for (auto i = 0; i < 100; ++i) keys.push_back("key" + std::to_string(i));

  storages::redis::CommandControl cc{};
  cc.chunk_size = 11;

  auto result = client->Mget(std::move(keys), cc).Get();
  EXPECT_EQ(result.size(), 100);
  EXPECT_EQ(*result[0], "foo");
  EXPECT_EQ(*result[1], "bar");
}

UTEST_F(RedisClient, Unlink) {
  auto client = GetClient();
  client->Set("key0", "foo", {}).Get();
  client->Set("key1", "bar", {}).Get();
  client->Hmset("key2", {{"field1", "value1"}, {"field2", "value2"}}, {}).Get();

  // Remove single key
  auto removed_count = client->Unlink("key0", {}).Get();
  EXPECT_EQ(removed_count, 1);

  // Remove multiple keys
  removed_count =
      client->Unlink(std::vector<std::string>{"key1", "key2"}, {}).Get();
  EXPECT_EQ(removed_count, 2);

  // Missing key, nothing removed
  removed_count = client->Unlink("key1", {}).Get();
  EXPECT_EQ(removed_count, 0);
}

UTEST_F(RedisClient, Geosearch) {
  auto client = GetClient();
  if (!CheckVersion({6, 2, 0}))
    GTEST_SKIP() << "Geosearch command available since 6.2.0";
  client
      ->Geoadd("Sicily",
               {{13.361389, 38.115556, "Palermo"},
                {15.087269, 37.502669, "Catania"}},
               {})
      .Get();

  redis::GeosearchOptions options;
  options.unit = redis::GeosearchOptions::Unit::kKm;

  const auto lon = redis::Longitude(15);
  const auto lat = redis::Latitude(37);
  const auto width = redis::BoxWidth(200);
  const auto height = redis::BoxHeight(200);

  // FROMLONLAT BYRADIUS
  auto result = client->Geosearch("Sicily", lon, lat, 100, options, {}).Get();
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result[0].member, "Catania");

  result = client->Geosearch("Sicily", lon, lat, 200, options, {}).Get();
  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result[0].member, "Palermo");
  EXPECT_EQ(result[1].member, "Catania");

  // FROMLONLAT BYBOX
  result =
      client->Geosearch("Sicily", lon, lat, width, height, options, {}).Get();
  // FROMMEMBER BYRADIUS
  result = client->Geosearch("Sicily", "Palermo", 200, options, {}).Get();
  // FROMMEMBER BYBOX
  result =
      client->Geosearch("Sicily", "Palermo", width, height, options, {}).Get();
}

USERVER_NAMESPACE_END
