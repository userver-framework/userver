#include <storages/redis/client_redistest.hpp>

#include <algorithm>
#include <memory>
#include <string_view>
#include <tuple>
#include <vector>

#include <userver/engine/task/task.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

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

UTEST_F(RedisClientTest, Sample) { RedisClientSampleUsage(*GetClient()); }

UTEST_F(RedisClientTest, CancelRequest) {
  try {
    EXPECT_FALSE(RedisClientCancelRequest(*GetClient()));
  } catch (const redis::RequestCancelledException&) {
  }
}

UTEST_F(RedisClientTest, Lrem) {
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

UTEST_F(RedisClientTest, Lpushx) {
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

UTEST_F(RedisClientTest, SetGet) {
  auto client = GetClient();
  UEXPECT_NO_THROW(client->Set("key0", "foo", {}).Get());
  EXPECT_EQ(client->Get("key0", {}).Get(), "foo");
}

UTEST_F(RedisClientTest, Mget) {
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

UTEST_F(RedisClientTest, Unlink) {
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

UTEST_F(RedisClientTest, Geosearch) {
  Version since{6, 2, 0};
  if (!CheckVersion(since))
    GTEST_SKIP() << SkipMsgByVersion("Geosearch", since);

  auto client = GetClient();
  client->Geoadd("Sicily", {13.361389, 38.115556, "Palermo"}, {}).Get();
  client->Geoadd("Sicily", {15.087269, 37.502669, "Catania"}, {}).Get();

  storages::redis::GeosearchOptions options;
  options.unit = storages::redis::GeosearchOptions::Unit::kKm;
  options.withdist = true;
  options.withhash = true;
  options.withcoord = true;

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

UTEST_F(RedisClientTest, Append) {
  auto client = GetClient();
  EXPECT_EQ(client->Exists("key", {}).Get(), 0);
  EXPECT_EQ(client->Append("key", "Hello", {}).Get(), 5);
  EXPECT_EQ(client->Append("key", " World", {}).Get(), 11);
  EXPECT_EQ(client->Get("key", {}).Get(), "Hello World");
}

UTEST_F(RedisClientTest, Dbsize) {
  auto client = GetClient();
  EXPECT_EQ(client->Dbsize(0, {}).Get(), 0);
}

UTEST_F(RedisClientTest, Del) {
  auto client = GetClient();
  client->Set("key1", "Hello", {}).Get();
  EXPECT_EQ(client->Del("key1", {}).Get(), 1);
  client->Set("key1", "Hello", {}).Get();
  client->Set("key2", "World", {}).Get();
  EXPECT_EQ(client->Del(std::vector<std::string>{"key1", "key2"}, {}).Get(), 2);
}

UTEST_F(RedisClientTest, Eval) {
  auto client = GetClient();

  auto result = client
                    ->Eval<std::vector<std::string>>(
                        "return { KEYS[1], KEYS[2], ARGV[1], ARGV[2] }",
                        {"key1", "key2"}, {"arg1", "arg2"}, {})
                    .Get();
  EXPECT_EQ(result.size(), 4);
  EXPECT_EQ(result[0], "key1");
}

UTEST_F(RedisClientTest, EvalSha) {
  auto client = GetClient();

  auto sha =
      client->ScriptLoad("return { KEYS[1], KEYS[2], ARGV[1], ARGV[2] }", 0, {})
          .Get();
  auto result = client
                    ->EvalSha<std::vector<std::string>>(sha, {"key1", "key2"},
                                                        {"arg1", "arg2"}, {})
                    .Get();
  EXPECT_TRUE(result.HasValue());
  EXPECT_EQ(result.Get().size(), 4);
  auto result_array = result.Extract();
  EXPECT_EQ(result_array[0], "key1");
}

UTEST_F(RedisClientTest, Exists) {
  auto client = GetClient();
  client->Set("key1", "Hello", {}).Get();
  EXPECT_EQ(client->Exists("key1", {}).Get(), 1);
  EXPECT_EQ(client->Exists("nosuchkey", {}).Get(), 0);
  client->Set("key2", "World", {}).Get();
  EXPECT_EQ(
      client->Exists(std::vector<std::string>{"key1", "key2", "nosuchkey"}, {})
          .Get(),
      2);
}

UTEST_F(RedisClientTest, Expire) {
  auto client = GetClient();
  client->Set("mykey", "Hello", {}).Get();
  EXPECT_EQ(client->Expire("mykey", std::chrono::seconds(10), {}).Get(),
            storages::redis::ExpireReply::kTimeoutWasSet);
  EXPECT_EQ(client->Ttl("mykey", {}).Get().GetExpireSeconds(), 10);
  client->Set("mykey", "Hello World", {}).Get();
  EXPECT_FALSE(client->Ttl("mykey", {}).Get().KeyHasExpiration());
}

UTEST_F(RedisClientTest, Georadius) {
  auto client = GetClient();
  client
      ->Geoadd("Sicily",
               {{13.361389, 38.115556, "Palermo"},
                {15.087269, 37.502669, "Catania"}},
               {})
      .Get();

  storages::redis::GeoradiusOptions options;
  options.unit = storages::redis::GeoradiusOptions::Unit::kKm;
  options.withdist = true;
  options.withhash = true;
  options.withcoord = true;

  const auto lon = redis::Longitude(15);
  const auto lat = redis::Latitude(37);

  auto result = client->Georadius("Sicily", lon, lat, 200, options, {}).Get();
  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result[0].member, "Palermo");
  EXPECT_EQ(result[0].dist, 190.4424);
  EXPECT_EQ(result[1].member, "Catania");
  EXPECT_EQ(result[1].dist, 56.4413);
}

UTEST_F(RedisClientTest, Getset) {
  auto client = GetClient();
  EXPECT_EQ(client->Incr("key", {}).Get(), 1);
  EXPECT_EQ(client->Getset("key", "0", {}).Get(), "1");
  EXPECT_EQ(client->Get("key", {}).Get(), "0");
}

UTEST_F(RedisClientTest, Hdel) {
  auto client = GetClient();
  // EXPECT_EQ(client->Hset("hash", "field1", "foo", {}).Get(),
  //           storages::redis::HsetReply::kCreated);
  client->Hset("hash", "field1", "foo", {}).Get();
  EXPECT_EQ(client->Hdel("hash", "field1", {}).Get(), 1);
  EXPECT_EQ(client->Hdel("hash", "field1", {}).Get(), 0);
  EXPECT_EQ(client->Hdel("hash", "field2", {}).Get(), 0);
}

UTEST_F(RedisClientTest, HdelMultiple) {
  auto client = GetClient();
  EXPECT_EQ(client->Hset("hash", "field1", "foo", {}).Get(),
            storages::redis::HsetReply::kCreated);
  EXPECT_EQ(
      client->Hdel("hash", std::vector<std::string>{"field1", "field2"}, {})
          .Get(),
      1);
}

UTEST_F(RedisClientTest, Hexists) {
  auto client = GetClient();
  client->Hset("hash", "field1", "foo", {}).Get();
  EXPECT_EQ(client->Hexists("hash", "field1", {}).Get(), 1);
  EXPECT_EQ(client->Hexists("hash", "field2", {}).Get(), 0);
}

UTEST_F(RedisClientTest, Hget) {
  auto client = GetClient();
  client->Hset("hash", "field1", "foo", {}).Get();
  auto result = client->Hget("hash", "field1", {}).Get();
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "foo");
  EXPECT_FALSE(client->Hget("hash", "field2", {}).Get().has_value());
}

UTEST_F(RedisClientTest, Hgetall) {
  auto client = GetClient();
  client->Hset("hash", "field1", "Hello", {}).Get();
  client->Hset("hash", "field2", "World", {}).Get();
  auto result = client->Hgetall("hash", {}).Get();
  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result["field1"], "Hello");
  EXPECT_EQ(result["field2"], "World");
}

UTEST_F(RedisClientTest, Hincrby) {
  auto client = GetClient();
  client->Hset("hash", "field1", "5", {}).Get();
  EXPECT_EQ(client->Hincrby("hash", "field1", 10, {}).Get(), 15);
}

UTEST_F(RedisClientTest, Hincrbyfloat) {
  auto client = GetClient();
  client->Hset("hash", "field1", "10.5", {}).Get();
  EXPECT_EQ(client->Hincrbyfloat("hash", "field1", 0.1, {}).Get(), 10.6);
}

UTEST_F(RedisClientTest, Hkeys) {
  auto client = GetClient();
  client->Hset("hash", "field1", "Hello", {}).Get();
  client->Hset("hash", "field2", "World", {}).Get();
  auto result = client->Hkeys("hash", {}).Get();
  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], "field1");
  EXPECT_EQ(result[1], "field2");
}

UTEST_F(RedisClientTest, Hlen) {
  auto client = GetClient();
  client->Hset("hash", "field1", "Hello", {}).Get();
  client->Hset("hash", "field2", "World", {}).Get();
  EXPECT_EQ(client->Hlen("hash", {}).Get(), 2);
}

UTEST_F(RedisClientTest, Hmget) {
  auto client = GetClient();
  client->Hset("hash", "field1", "Hello", {}).Get();
  client->Hset("hash", "field2", "World", {}).Get();
  auto result =
      client
          ->Hmget("hash",
                  std::vector<std::string>{"field1", "field2", "nofield"}, {})
          .Get();
  EXPECT_EQ(result.size(), 3);
  EXPECT_TRUE(result[0].has_value());
  EXPECT_TRUE(result[1].has_value());
  EXPECT_FALSE(result[2].has_value());
  EXPECT_EQ(result[0].value(), "Hello");
  EXPECT_EQ(result[1].value(), "World");
}

UTEST_F(RedisClientTest, Hmset) {
  auto client = GetClient();
  UEXPECT_NO_THROW(client
                       ->Hmset("hash",
                               std::vector<std::pair<std::string, std::string>>{
                                   {"field1", "1"}, {"field2", "2"}},
                               {})
                       .Get());
  auto result = client->Hget("hash", "field1", {}).Get();
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "1");
  result = client->Hget("hash", "field2", {}).Get();
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "2");
}

UTEST_F(RedisClientTest, Hset) {
  auto client = GetClient();

  EXPECT_EQ(client->Hset("hash", "field", "Hello", {}).Get(),
            storages::redis::HsetReply::kCreated);
  auto result = client->Hget("hash", "field", {}).Get();
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "Hello");
  EXPECT_EQ(client->Hset("hash", "field", "World", {}).Get(),
            storages::redis::HsetReply::kUpdated);
}

UTEST_F(RedisClientTest, Hsetnx) {
  auto client = GetClient();

  EXPECT_TRUE(client->Hsetnx("hash", "field", "Hello", {}).Get());
  EXPECT_FALSE(client->Hsetnx("hash", "field", "World", {}).Get());
  auto result = client->Hget("hash", "field", {}).Get();
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "Hello");
}

UTEST_F(RedisClientTest, Hvals) {
  auto client = GetClient();

  client->Hset("hash", "field1", "Hello", {}).Get();
  client->Hset("hash", "field2", "World", {}).Get();
  auto result = client->Hvals("hash", {}).Get();
  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], "Hello");
  EXPECT_EQ(result[1], "World");
}

UTEST_F(RedisClientTest, Incr) {
  auto client = GetClient();

  client->Set("key", "10", {}).Get();
  client->Incr("key", {}).Get();
  auto result = client->Get("key", {}).Get();
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "11");
}

UTEST_F(RedisClientTest, Lindex) {
  auto client = GetClient();

  client->Lpush("list", "World", {}).Get();
  client->Lpush("list", "Hello", {}).Get();
  auto result = client->Lindex("list", 0, {}).Get();
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "Hello");
  result = client->Lindex("list", -1, {}).Get();
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "World");
  result = client->Lindex("list", 3, {}).Get();
  EXPECT_FALSE(result.has_value());
}

UTEST_F(RedisClientTest, Llen) {
  auto client = GetClient();

  client->Lpush("list", "World", {}).Get();
  client->Lpush("list", "Hello", {}).Get();

  EXPECT_EQ(client->Llen("list", {}).Get(), 2);
}

UTEST_F(RedisClientTest, Lpop) {
  auto client = GetClient();

  client->Rpush("list", {"1", "2", "3", "4"}, {}).Get();
  auto result = client->Lpop("list", {}).Get();
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "1");
  EXPECT_EQ(client->Llen("list", {}).Get(), 3);
}

UTEST_F(RedisClientTest, Lpush) {
  auto client = GetClient();

  EXPECT_EQ(client->Lpush("list", "World", {}).Get(), 1);
  EXPECT_EQ(client->Lpush("list", "Hello", {}).Get(), 2);
  EXPECT_EQ(
      client->Lpush("list", std::vector<std::string>{"v1", "v2"}, {}).Get(), 4);

  auto result = client->Lrange("list", 0, -1, {}).Get();
  EXPECT_EQ(result.size(), 4);
  EXPECT_EQ(result[0], "v2");
}

UTEST_F(RedisClientTest, Lrange) {
  auto client = GetClient();

  client->Rpush("list", "one", {}).Get();
  client->Rpush("list", "two", {}).Get();
  client->Rpush("list", "three", {}).Get();

  auto result = client->Lrange("list", -3, 2, {}).Get();
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], "one");
}

UTEST_F(RedisClientTest, Ltrim) {
  auto client = GetClient();

  client->Rpush("list", "one", {}).Get();
  client->Rpush("list", "two", {}).Get();
  client->Rpush("list", "three", {}).Get();

  EXPECT_NO_THROW(client->Ltrim("list", 1, -1, {}).Get());
  auto result = client->Lrange("list", 0, -1, {}).Get();
  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], "two");
}

UTEST_F(RedisClientTest, Mset) {
  auto client = GetClient();

  EXPECT_NO_THROW(
      client->Mset({{"key1", "value1"}, {"key2", "value2"}}, {}).Get());
  auto result = client->Get("key1", {}).Get();
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "value1");
}

// multi

UTEST_F(RedisClientTest, Persist) {
  auto client = GetClient();

  client->Set("key", "Hello", {}).Get();
  client->Expire("key", std::chrono::seconds{10}, {}).Get();
  EXPECT_TRUE(client->Ttl("key", {}).Get().KeyHasExpiration());
  EXPECT_EQ(client->Persist("key", {}).Get(),
            storages::redis::PersistReply::kTimeoutRemoved);
  EXPECT_FALSE(client->Ttl("key", {}).Get().KeyHasExpiration());
}

UTEST_F(RedisClientTest, Pexpire) {
  auto client = GetClient();

  client->Set("key", "Hello", {}).Get();
  EXPECT_EQ(client->Pexpire("key", std::chrono::milliseconds{1999}, {}).Get(),
            storages::redis::ExpireReply::kTimeoutWasSet);
  EXPECT_EQ(client->Ttl("key", {}).Get().GetExpireSeconds(), 2);
}

UTEST_F(RedisClientTest, Ping) {
  auto client = GetClient();

  EXPECT_NO_THROW(client->Ping(0, {}).Get());
  EXPECT_EQ(client->Ping(0, "Ping", {}).Get(), "Ping");
}

// Publish

UTEST_F(RedisClientTest, Rename) {
  auto client = GetClient();

  client->Set("key", "value", {}).Get();
  EXPECT_NO_THROW(client->Rename("key", "new key", {}).Get());
  auto result = client->Get("new key", {}).Get();
  EXPECT_TRUE(result.has_value());
}

UTEST_F(RedisClientTest, Rpop) {
  auto client = GetClient();

  client->Rpush("list", std::vector<std::string>{"1", "2"}, {}).Get();
  auto result = client->Rpop("list", {}).Get();
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "2");
  client->Rpop("list", {}).Get();
  result = client->Rpop("list", {}).Get();
  EXPECT_FALSE(result.has_value());
}

UTEST_F(RedisClientTest, Rpush) {
  auto client = GetClient();

  EXPECT_EQ(client->Rpush("list", std::vector<std::string>{"1", "2"}, {}).Get(),
            2);
  EXPECT_EQ(client->Rpush("list", "3", {}).Get(), 3);
  auto result = client->Lrange("list", 0, -1, {}).Get();
  EXPECT_EQ(result.size(), 3);
}

UTEST_F(RedisClientTest, Rpushx) {
  auto client = GetClient();

  client->Rpush("list", "1", {}).Get();
  EXPECT_EQ(client->Rpushx("list", "2", {}).Get(), 2);
  EXPECT_EQ(client->Rpushx("other list", "1", {}).Get(), 0);
}

UTEST_F(RedisClientTest, Sadd) {
  auto client = GetClient();

  EXPECT_EQ(client->Sadd("set", "hello", {}).Get(), 1);
  EXPECT_EQ(
      client->Sadd("set", std::vector<std::string>{"world", "world"}, {}).Get(),
      1);
  auto result = client->Smembers("set", {}).Get();
  EXPECT_EQ(result.size(), 2);
}

UTEST_F(RedisClientTest, Scard) {
  auto client = GetClient();

  client->Sadd("set", "hello", {}).Get();
  client->Sadd("set", "set", {}).Get();
  EXPECT_EQ(client->Scard("set", {}).Get(), 2);
}

UTEST_F(RedisClientTest, Set) {
  auto client = GetClient();

  EXPECT_NO_THROW(client->Set("key", "value", {}).Get());
  auto result = client->Get("key", {}).Get();
  EXPECT_TRUE(result.has_value());
  EXPECT_NO_THROW(
      client->Set("another key", "value", std::chrono::milliseconds{1999}, {})
          .Get());
  EXPECT_TRUE(client->Ttl("another key", {}).Get().KeyHasExpiration());

  EXPECT_TRUE(client->SetIfExist("key", "new value", {}).Get());
  EXPECT_FALSE(client->SetIfExist("new key", "value", {}).Get());
  EXPECT_FALSE(client->SetIfNotExist("key", "new value", {}).Get());
  EXPECT_TRUE(client->SetIfNotExist("new key", "value", {}).Get());
}

UTEST_F(RedisClientTest, Setex) {
  auto client = GetClient();

  EXPECT_NO_THROW(
      client->Setex("key", std::chrono::seconds{10}, "value", {}).Get());
  EXPECT_EQ(client->Ttl("key", {}).Get().GetExpireSeconds(), 10);
}

UTEST_F(RedisClientTest, Sismember) {
  auto client = GetClient();

  client->Sadd("key", "one", {}).Get();
  EXPECT_EQ(client->Sismember("key", "one", {}).Get(), 1);
  EXPECT_EQ(client->Sismember("key", "two", {}).Get(), 0);
}

UTEST_F(RedisClientTest, Smembers) {
  auto client = GetClient();

  client->Sadd("set", std::vector<std::string>{"world", "world", "hello"}, {})
      .Get();
  auto result = client->Smembers("set", {}).Get();
  EXPECT_EQ(result.size(), 2);
}

UTEST_F(RedisClientTest, Srandmember) {
  auto client = GetClient();

  client->Sadd("set", std::vector<std::string>{"one", "two", "three"}, {})
      .Get();
  auto result1 = client->Srandmember("set", {}).Get();
  EXPECT_TRUE(result1.has_value());
  EXPECT_TRUE(result1 == "one" || result1 == "two" || result1 == "three");

  auto result2 = client->Srandmembers("set", 2, {}).Get();
  EXPECT_EQ(result2.size(), 2);
  result2 = client->Srandmembers("set", -5, {}).Get();
  EXPECT_EQ(result2.size(), 5);
}

UTEST_F(RedisClientTest, Srem) {
  auto client = GetClient();

  client
      ->Sadd("set", std::vector<std::string>{"one", "two", "three", "four"}, {})
      .Get();
  EXPECT_EQ(client->Srem("set", "one", {}).Get(), 1);

  EXPECT_EQ(
      client->Srem("set", std::vector<std::string>{"two", "three", "five"}, {})
          .Get(),
      2);
}

UTEST_F(RedisClientTest, Strlen) {
  auto client = GetClient();

  client->Set("key", "value", {}).Get();
  EXPECT_EQ(client->Strlen("key", {}).Get(), 5);
  EXPECT_EQ(client->Strlen("nonexisting", {}).Get(), 0);
}

UTEST_F(RedisClientTest, Time) {
  auto client = GetClient();

  client->Time(0, {}).Get();
}

UTEST_F(RedisClientTest, Type) {
  auto client = GetClient();

  client->Set("key1", "value", {}).Get();
  client->Lpush("key2", "value", {}).Get();
  EXPECT_EQ(client->Type("key1", {}).Get(), storages::redis::KeyType::kString);
  EXPECT_EQ(client->Type("key2", {}).Get(), storages::redis::KeyType::kList);
}

UTEST_F(RedisClientTest, Zadd) {
  auto client = GetClient();

  EXPECT_EQ(client->Zadd("zset", 1., "one", {}).Get(), 1);
  storages::redis::ZaddOptions options;
  options.exist = storages::redis::ZaddOptions::Exist::kAddIfNotExist;
  EXPECT_EQ(client->Zadd("zset", 2., "two", options, {}).Get(), 1);

  EXPECT_EQ(client->Zadd("zset", {{2., "two"}, {3., "three"}}, {}).Get(), 1);

  options.exist = storages::redis::ZaddOptions::Exist::kAddIfExist;
  options.return_value =
      storages::redis::ZaddOptions::ReturnValue::kChangedCount;
  EXPECT_EQ(
      client->Zadd("zset", {{2.5, "two"}, {3.5, "three"}}, options, {}).Get(),
      2);
  auto result = client->ZrangeWithScores("zset", 0, -1, {}).Get();
  EXPECT_EQ(result[1].score, 2.5);

  EXPECT_EQ(client->ZaddIncr("zset", 1.2, "two", {}).Get(), 3.7);
  EXPECT_EQ(client->ZaddIncr("zset", 4., "four", {}).Get(), 4.);
  EXPECT_EQ(client->ZaddIncrExisting("zset", 1.1, "four", {}).Get(), 5.1);
  EXPECT_FALSE(
      client->ZaddIncrExisting("zset", 1.1, "five", {}).Get().has_value());
}

UTEST_F(RedisClientTest, Zcard) {
  auto client = GetClient();

  client->Zadd("zset", {{2., "two"}, {3., "three"}}, {}).Get();
  EXPECT_EQ(client->Zcard("zset", {}).Get(), 2);
}

UTEST_F(RedisClientTest, Zrange) {
  auto client = GetClient();

  client->Zadd("zset", {{2., "two"}, {3., "three"}, {1., "one"}}, {}).Get();
  auto result = client->Zrange("zset", 0, -1, {}).Get();
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], "one");
  auto result_with_scores = client->ZrangeWithScores("zset", 0, -1, {}).Get();
  EXPECT_EQ(result_with_scores.size(), 3);
  EXPECT_EQ(result_with_scores[0].score, 1.);
}

UTEST_F(RedisClientTest, Zrangebyscore) {
  auto client = GetClient();

  client
      ->Zadd(
          "zset",
          {{2., "two"}, {3., "three"}, {1., "one"}, {4., "four"}, {5., "five"}},
          {})
      .Get();

  storages::redis::RangeOptions options;
  options.offset = 1;
  options.count = 10;

  auto result1 = client->Zrangebyscore("zset", 2., 4., {}).Get();
  EXPECT_EQ(result1.size(), 3);
  EXPECT_EQ(result1[0], "two");
  auto result2 = client->Zrangebyscore("zset", 2., 4., options, {}).Get();
  EXPECT_EQ(result2.size(), 2);
  EXPECT_EQ(result2[0], "three");

  auto result1_scores =
      client->ZrangebyscoreWithScores("zset", 2., 4., {}).Get();
  EXPECT_EQ(result1_scores.size(), 3);
  EXPECT_EQ(result1_scores[0].member, "two");
  auto result2_scores =
      client->ZrangebyscoreWithScores("zset", 2., 4., options, {}).Get();
  EXPECT_EQ(result2_scores.size(), 2);
  EXPECT_EQ(result2_scores[0].member, "three");
}

UTEST_F(RedisClientTest, ZrangebyscoreString) {
  auto client = GetClient();

  client
      ->Zadd(
          "zset",
          {{2., "two"}, {3., "three"}, {1., "one"}, {4., "four"}, {5., "five"}},
          {})
      .Get();

  storages::redis::RangeOptions options;
  options.offset = 1;
  options.count = 10;

  auto result1 = client->Zrangebyscore("zset", "(2.0", "4.0", {}).Get();
  EXPECT_EQ(result1.size(), 2);
  EXPECT_EQ(result1[0], "three");
  auto result2 =
      client->Zrangebyscore("zset", "2.0", "(5.0", options, {}).Get();
  EXPECT_EQ(result2.size(), 2);
  EXPECT_EQ(result2[0], "three");
  auto result1_scores =
      client->ZrangebyscoreWithScores("zset", "(2.0", "4.0", {}).Get();
  EXPECT_EQ(result1_scores.size(), 2);
  EXPECT_EQ(result1_scores[0].member, "three");
  auto result2_scores =
      client->ZrangebyscoreWithScores("zset", "2.0", "(5.0", options, {}).Get();
  EXPECT_EQ(result2_scores.size(), 2);
  EXPECT_EQ(result2_scores[0].member, "three");
}

UTEST_F(RedisClientTest, Zrem) {
  auto client = GetClient();

  client
      ->Zadd(
          "zset",
          {{2., "two"}, {3., "three"}, {1., "one"}, {4., "four"}, {5., "five"}},
          {})
      .Get();
  EXPECT_EQ(client->Zrem("zset", "two", {}).Get(), 1);
  EXPECT_EQ(
      client->Zrem("zset", std::vector<std::string>{"two", "one", "three"}, {})
          .Get(),
      2);
  auto result = client->Zrange("zset", 0, -1, {}).Get();
  EXPECT_EQ(result.size(), 2);
}

UTEST_F(RedisClientTest, Zremrangebyrank) {
  auto client = GetClient();

  client->Zadd("zset", {{2., "two"}, {3., "three"}, {1., "one"}}, {}).Get();
  EXPECT_EQ(client->Zremrangebyrank("zset", 0, 1, {}).Get(), 2);
  auto result = client->Zrange("zset", 0, -1, {}).Get();
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], "three");
}

UTEST_F(RedisClientTest, Zremrangebyscore) {
  auto client = GetClient();

  client->Zadd("zset", {{2., "two"}, {3., "three"}, {1., "one"}}, {}).Get();
  EXPECT_EQ(client->Zremrangebyscore("zset", 1., 2., {}).Get(), 2);
  auto result = client->Zrange("zset", 0, -1, {}).Get();
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], "three");

  client->Zadd("zset", {{2., "two"}, {3., "three"}, {1., "one"}}, {}).Get();
  EXPECT_EQ(client->Zremrangebyscore("zset", "-inf", "(3", {}).Get(), 2);
  result = client->Zrange("zset", 0, -1, {}).Get();
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], "three");
}

UTEST_F(RedisClientTest, Zscore) {
  auto client = GetClient();

  client->Zadd("zset", 2., "two", {}).Get();
  EXPECT_EQ(client->Zscore("zset", "two", {}).Get(), 2.);
}

UTEST_F(RedisClientTest, TransactionType) {
  auto client = GetClient();
  auto transaction = client->Multi();

  auto set = transaction->Set("key1", "value");
  auto lpush = transaction->Lpush("key2", "value");
  auto type1 = transaction->Type("key1");
  auto type2 = transaction->Type("key2");
  transaction->Exec({}).Get();
  EXPECT_EQ(type1.Get(), storages::redis::KeyType::kString);
  EXPECT_EQ(type2.Get(), storages::redis::KeyType::kList);
}

UTEST_F(RedisClientTest, TransactionZrem) {
  auto client = GetClient();
  auto transaction = client->Multi();

  auto zadd1 = transaction->Zadd(
      "zset",
      {{2., "two"}, {3., "three"}, {1., "one"}, {4., "four"}, {5., "five"}});
  auto zrem1 = transaction->Zrem("zset", "two");
  auto zrem2 = transaction->Zrem(
      "zset", std::vector<std::string>{"two", "one", "three"});
  auto zrange = transaction->Zrange("zset", 0, -1);
  transaction->Exec({}).Get();

  EXPECT_EQ(zrem1.Get(), 1);
  EXPECT_EQ(zrem2.Get(), 2);
  auto result = zrange.Get();
  EXPECT_EQ(result.size(), 2);
}

USERVER_NAMESPACE_END
