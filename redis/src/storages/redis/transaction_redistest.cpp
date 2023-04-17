#include <userver/utest/utest.hpp>

#include <algorithm>

#include <storages/redis/client_redistest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
redis::CommandControl kDefaultCc(std::chrono::milliseconds(500),
                                 std::chrono::milliseconds(500), 1);

class RedisClientTransactionTest : public RedisClientTest {
 public:
  auto& GetTransactionClient() {
    transaction_ = GetClient()->Multi();
    return transaction_;
  }

  template <typename Result, typename ReplyType = Result>
  ReplyType Get(storages::redis::Request<Result, ReplyType>&& request) {
    transaction_->Exec(kDefaultCc).Get();
    auto result = request.Get();
    transaction_ = GetClient()->Multi();
    return result;
  }

  template <typename Result>
  void Get(storages::redis::Request<Result, void>&&) {
    transaction_->Exec(kDefaultCc).Get();
    transaction_ = GetClient()->Multi();
  }

 private:
  storages::redis::TransactionPtr transaction_;
};
}  // namespace

UTEST_F(RedisClientTransactionTest, Lrem) {
  auto& client = GetTransactionClient();

  EXPECT_EQ(Get(client->Rpush("testlist", "a")), 1);
  EXPECT_EQ(Get(client->Rpush("testlist", "b")), 2);
  EXPECT_EQ(Get(client->Rpush("testlist", "a")), 3);
  EXPECT_EQ(Get(client->Rpush("testlist", "b")), 4);
  EXPECT_EQ(Get(client->Rpush("testlist", "b")), 5);
  EXPECT_EQ(Get(client->Llen("testlist")), 5);

  EXPECT_EQ(Get(client->Lrem("testlist", 1, "b")), 1);
  EXPECT_EQ(Get(client->Llen("testlist")), 4);
  EXPECT_EQ(Get(client->Lrem("testlist", -2, "b")), 2);
  EXPECT_EQ(Get(client->Llen("testlist")), 2);
  EXPECT_EQ(Get(client->Lrem("testlist", 0, "c")), 0);
  EXPECT_EQ(Get(client->Llen("testlist")), 2);
  EXPECT_EQ(Get(client->Lrem("testlist", 0, "a")), 2);
  EXPECT_EQ(Get(client->Llen("testlist")), 0);
}

UTEST_F(RedisClientTransactionTest, Lpushx) {
  auto& client = GetTransactionClient();
  // Missing array - does nothing
  EXPECT_EQ(Get(client->Lpushx("pushx_testlist", "a")), 0);
  EXPECT_EQ(Get(client->Rpushx("pushx_testlist", "a")), 0);
  // Init list
  EXPECT_EQ(Get(client->Lpush("pushx_testlist", "a")), 1);
  // List exists - appends values
  EXPECT_EQ(Get(client->Lpushx("pushx_testlist", "a")), 2);
  EXPECT_EQ(Get(client->Rpushx("pushx_testlist", "a")), 3);
}

UTEST_F(RedisClientTransactionTest, SetGet) {
  auto& client = GetTransactionClient();
  UEXPECT_NO_THROW(Get(client->Set("key0", "foo")));
  EXPECT_EQ(Get(client->Get("key0")), "foo");
}

UTEST_F(RedisClientTransactionTest, Mget) {
  auto& client = GetTransactionClient();
  Get(client->Set("key0", "foo"));
  Get(client->Set("key1", "bar"));

  std::vector<std::string> keys;
  keys.reserve(100);
  for (auto i = 0; i < 100; ++i) keys.push_back("key" + std::to_string(i));

  storages::redis::CommandControl cc{};
  cc.chunk_size = 11;

  auto result = Get(client->Mget(std::move(keys)));
  EXPECT_EQ(result.size(), 100);
  EXPECT_EQ(*result[0], "foo");
  EXPECT_EQ(*result[1], "bar");
}

UTEST_F(RedisClientTransactionTest, Unlink) {
  auto& client = GetTransactionClient();
  Get(client->Set("key0", "foo"));
  Get(client->Set("key1", "bar"));
  Get(client->Hmset("key2", {{"field1", "value1"}, {"field2", "value2"}}));

  // Remove single key
  auto removed_count = Get(client->Unlink("key0"));
  EXPECT_EQ(removed_count, 1);

  // Remove multiple keys
  removed_count = Get(client->Unlink(std::vector<std::string>{"key1", "key2"}));
  EXPECT_EQ(removed_count, 2);

  // Missing key, nothing removed
  removed_count = Get(client->Unlink("key1"));
  EXPECT_EQ(removed_count, 0);
}

UTEST_F(RedisClientTransactionTest, Geosearch) {
  Version since{6, 2, 0};
  if (!CheckVersion(since))
    GTEST_SKIP() << SkipMsgByVersion("Geosearch", since);

  auto& client = GetTransactionClient();
  Get(client->Geoadd("Sicily", {13.361389, 38.115556, "Palermo"}));
  Get(client->Geoadd("Sicily", {15.087269, 37.502669, "Catania"}));

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
  auto result = Get(client->Geosearch("Sicily", lon, lat, 100, options));
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result[0].member, "Catania");

  result = Get(client->Geosearch("Sicily", lon, lat, 200, options));
  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result[0].member, "Palermo");
  EXPECT_EQ(result[1].member, "Catania");

  // FROMLONLAT BYBOX
  result = Get(client->Geosearch("Sicily", lon, lat, width, height, options));
  // FROMMEMBER BYRADIUS
  result = Get(client->Geosearch("Sicily", "Palermo", 200, options));
  // FROMMEMBER BYBOX
  result = Get(client->Geosearch("Sicily", "Palermo", width, height, options));
}

UTEST_F(RedisClientTransactionTest, Append) {
  auto& client = GetTransactionClient();
  EXPECT_EQ(Get(client->Exists("key")), 0);
  EXPECT_EQ(Get(client->Append("key", "Hello")), 5);
  EXPECT_EQ(Get(client->Append("key", " World")), 11);
  EXPECT_EQ(Get(client->Get("key")), "Hello World");
}

UTEST_F(RedisClientTransactionTest, Dbsize) {
  auto& client = GetTransactionClient();
  EXPECT_EQ(Get(client->Dbsize(0)), 0);
}

UTEST_F(RedisClientTransactionTest, Del) {
  auto& client = GetTransactionClient();
  Get(client->Set("key1", "Hello"));
  EXPECT_EQ(Get(client->Del("key1")), 1);
  Get(client->Set("key1", "Hello"));
  Get(client->Set("key2", "World"));
  EXPECT_EQ(Get(client->Del(std::vector<std::string>{"key1", "key2"})), 2);
}

UTEST_F(RedisClientTransactionTest, Exists) {
  auto& client = GetTransactionClient();
  Get(client->Set("key1", "Hello"));
  EXPECT_EQ(Get(client->Exists("key1")), 1);
  EXPECT_EQ(Get(client->Exists("nosuchkey")), 0);
  Get(client->Set("key2", "World"));
  EXPECT_EQ(Get(client->Exists(
                std::vector<std::string>{"key1", "key2", "nosuchkey"})),
            2);
}

UTEST_F(RedisClientTransactionTest, Expire) {
  auto& client = GetTransactionClient();
  Get(client->Set("mykey", "Hello"));
  EXPECT_EQ(Get(client->Expire("mykey", std::chrono::seconds(10))),
            storages::redis::ExpireReply::kTimeoutWasSet);
  EXPECT_EQ(Get(client->Ttl("mykey")).GetExpireSeconds(), 10);
  Get(client->Set("mykey", "Hello World"));
  EXPECT_FALSE(Get(client->Ttl("mykey")).KeyHasExpiration());
}

UTEST_F(RedisClientTransactionTest, Georadius) {
  auto& client = GetTransactionClient();
  Get(client->Geoadd("Sicily", {{13.361389, 38.115556, "Palermo"},
                                {15.087269, 37.502669, "Catania"}}));

  storages::redis::GeoradiusOptions options;
  options.unit = storages::redis::GeoradiusOptions::Unit::kKm;
  options.withdist = true;
  options.withhash = true;
  options.withcoord = true;

  const auto lon = redis::Longitude(15);
  const auto lat = redis::Latitude(37);

  auto result = Get(client->Georadius("Sicily", lon, lat, 200, options));
  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result[0].member, "Palermo");
  EXPECT_EQ(result[0].dist, 190.4424);
  EXPECT_EQ(result[1].member, "Catania");
  EXPECT_EQ(result[1].dist, 56.4413);
}

UTEST_F(RedisClientTransactionTest, Getset) {
  auto& client = GetTransactionClient();
  EXPECT_EQ(Get(client->Incr("key")), 1);
  EXPECT_EQ(Get(client->Getset("key", "0")), "1");
  EXPECT_EQ(Get(client->Get("key")), "0");
}

UTEST_F(RedisClientTransactionTest, Hdel) {
  auto& client = GetTransactionClient();
  // EXPECT_EQ(Get(client->Hset("hash", "field1", "foo")),
  //           storages::redis::HsetReply::kCreated);
  Get(client->Hset("hash", "field1", "foo"));
  EXPECT_EQ(Get(client->Hdel("hash", "field1")), 1);
  EXPECT_EQ(Get(client->Hdel("hash", "field1")), 0);
  EXPECT_EQ(Get(client->Hdel("hash", "field2")), 0);
}

UTEST_F(RedisClientTransactionTest, HdelMultiple) {
  auto& client = GetTransactionClient();
  EXPECT_EQ(Get(client->Hset("hash", "field1", "foo")),
            storages::redis::HsetReply::kCreated);
  EXPECT_EQ(
      Get(client->Hdel("hash", std::vector<std::string>{"field1", "field2"})),
      1);
}

UTEST_F(RedisClientTransactionTest, Hexists) {
  auto& client = GetTransactionClient();
  Get(client->Hset("hash", "field1", "foo"));
  EXPECT_EQ(Get(client->Hexists("hash", "field1")), 1);
  EXPECT_EQ(Get(client->Hexists("hash", "field2")), 0);
}

UTEST_F(RedisClientTransactionTest, Hget) {
  auto& client = GetTransactionClient();
  Get(client->Hset("hash", "field1", "foo"));
  auto result = Get(client->Hget("hash", "field1"));
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "foo");
  EXPECT_FALSE(Get(client->Hget("hash", "field2")).has_value());
}

UTEST_F(RedisClientTransactionTest, Hgetall) {
  auto& client = GetTransactionClient();
  Get(client->Hset("hash", "field1", "Hello"));
  Get(client->Hset("hash", "field2", "World"));
  auto result = Get(client->Hgetall("hash"));
  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result["field1"], "Hello");
  EXPECT_EQ(result["field2"], "World");
}

UTEST_F(RedisClientTransactionTest, Hincrby) {
  auto& client = GetTransactionClient();
  Get(client->Hset("hash", "field1", "5"));
  EXPECT_EQ(Get(client->Hincrby("hash", "field1", 10)), 15);
}

UTEST_F(RedisClientTransactionTest, Hincrbyfloat) {
  auto& client = GetTransactionClient();
  Get(client->Hset("hash", "field1", "10.5"));
  EXPECT_EQ(Get(client->Hincrbyfloat("hash", "field1", 0.1)), 10.6);
}

UTEST_F(RedisClientTransactionTest, Hkeys) {
  auto& client = GetTransactionClient();
  Get(client->Hset("hash", "field1", "Hello"));
  Get(client->Hset("hash", "field2", "World"));
  auto result = Get(client->Hkeys("hash"));
  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], "field1");
  EXPECT_EQ(result[1], "field2");
}

UTEST_F(RedisClientTransactionTest, Hlen) {
  auto& client = GetTransactionClient();
  Get(client->Hset("hash", "field1", "Hello"));
  Get(client->Hset("hash", "field2", "World"));
  EXPECT_EQ(Get(client->Hlen("hash")), 2);
}

UTEST_F(RedisClientTransactionTest, Hmget) {
  auto& client = GetTransactionClient();
  Get(client->Hset("hash", "field1", "Hello"));
  Get(client->Hset("hash", "field2", "World"));
  auto result = Get(client->Hmget(
      "hash", std::vector<std::string>{"field1", "field2", "nofield"}));
  EXPECT_EQ(result.size(), 3);
  EXPECT_TRUE(result[0].has_value());
  EXPECT_TRUE(result[1].has_value());
  EXPECT_FALSE(result[2].has_value());
  EXPECT_EQ(result[0].value(), "Hello");
  EXPECT_EQ(result[1].value(), "World");
}

UTEST_F(RedisClientTransactionTest, Hmset) {
  auto& client = GetTransactionClient();
  UEXPECT_NO_THROW(Get(
      client->Hmset("hash", std::vector<std::pair<std::string, std::string>>{
                                {"field1", "1"}, {"field2", "2"}})));
  auto result = Get(client->Hget("hash", "field1"));
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "1");
  result = Get(client->Hget("hash", "field2"));
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "2");
}

UTEST_F(RedisClientTransactionTest, Hset) {
  auto& client = GetTransactionClient();

  EXPECT_EQ(Get(client->Hset("hash", "field", "Hello")),
            storages::redis::HsetReply::kCreated);
  auto result = Get(client->Hget("hash", "field"));
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "Hello");
  EXPECT_EQ(Get(client->Hset("hash", "field", "World")),
            storages::redis::HsetReply::kUpdated);
}

UTEST_F(RedisClientTransactionTest, Hsetnx) {
  auto& client = GetTransactionClient();

  EXPECT_TRUE(Get(client->Hsetnx("hash", "field", "Hello")));
  EXPECT_FALSE(Get(client->Hsetnx("hash", "field", "World")));
  auto result = Get(client->Hget("hash", "field"));
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "Hello");
}

UTEST_F(RedisClientTransactionTest, Hvals) {
  auto& client = GetTransactionClient();

  Get(client->Hset("hash", "field1", "Hello"));
  Get(client->Hset("hash", "field2", "World"));
  auto result = Get(client->Hvals("hash"));
  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], "Hello");
  EXPECT_EQ(result[1], "World");
}

UTEST_F(RedisClientTransactionTest, Incr) {
  auto& client = GetTransactionClient();

  Get(client->Set("key", "10"));
  Get(client->Incr("key"));
  auto result = Get(client->Get("key"));
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "11");
}

UTEST_F(RedisClientTransactionTest, Lindex) {
  auto& client = GetTransactionClient();

  Get(client->Lpush("list", "World"));
  Get(client->Lpush("list", "Hello"));
  auto result = Get(client->Lindex("list", 0));
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "Hello");
  result = Get(client->Lindex("list", -1));
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "World");
  result = Get(client->Lindex("list", 3));
  EXPECT_FALSE(result.has_value());
}

UTEST_F(RedisClientTransactionTest, Llen) {
  auto& client = GetTransactionClient();

  Get(client->Lpush("list", "World"));
  Get(client->Lpush("list", "Hello"));

  EXPECT_EQ(Get(client->Llen("list")), 2);
}

UTEST_F(RedisClientTransactionTest, Lpop) {
  auto& client = GetTransactionClient();

  Get(client->Rpush("list", {"1", "2", "3", "4"}));
  auto result = Get(client->Lpop("list"));
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "1");
  EXPECT_EQ(Get(client->Llen("list")), 3);
}

UTEST_F(RedisClientTransactionTest, Lpush) {
  auto& client = GetTransactionClient();

  EXPECT_EQ(Get(client->Lpush("list", "World")), 1);
  EXPECT_EQ(Get(client->Lpush("list", "Hello")), 2);
  EXPECT_EQ(Get(client->Lpush("list", std::vector<std::string>{"v1", "v2"})),
            4);

  auto result = Get(client->Lrange("list", 0, -1));
  EXPECT_EQ(result.size(), 4);
  EXPECT_EQ(result[0], "v2");
}

UTEST_F(RedisClientTransactionTest, Lrange) {
  auto& client = GetTransactionClient();

  Get(client->Rpush("list", "one"));
  Get(client->Rpush("list", "two"));
  Get(client->Rpush("list", "three"));

  auto result = Get(client->Lrange("list", -3, 2));
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], "one");
}

UTEST_F(RedisClientTransactionTest, Ltrim) {
  auto& client = GetTransactionClient();

  Get(client->Rpush("list", "one"));
  Get(client->Rpush("list", "two"));
  Get(client->Rpush("list", "three"));

  EXPECT_NO_THROW(Get(client->Ltrim("list", 1, -1)));
  auto result = Get(client->Lrange("list", 0, -1));
  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], "two");
}

UTEST_F(RedisClientTransactionTest, Mset) {
  auto& client = GetTransactionClient();

  EXPECT_NO_THROW(Get(client->Mset({{"key1", "value1"}, {"key2", "value2"}})));
  auto result = Get(client->Get("key1"));
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "value1");
}

// multi

UTEST_F(RedisClientTransactionTest, Persist) {
  auto& client = GetTransactionClient();

  Get(client->Set("key", "Hello"));
  Get(client->Expire("key", std::chrono::seconds{10}));
  EXPECT_TRUE(Get(client->Ttl("key")).KeyHasExpiration());
  EXPECT_EQ(Get(client->Persist("key")),
            storages::redis::PersistReply::kTimeoutRemoved);
  EXPECT_FALSE(Get(client->Ttl("key")).KeyHasExpiration());
}

UTEST_F(RedisClientTransactionTest, Pexpire) {
  auto& client = GetTransactionClient();

  Get(client->Set("key", "Hello"));
  EXPECT_EQ(Get(client->Pexpire("key", std::chrono::milliseconds{1999})),
            storages::redis::ExpireReply::kTimeoutWasSet);
  EXPECT_EQ(Get(client->Ttl("key")).GetExpireSeconds(), 2);
}

UTEST_F(RedisClientTransactionTest, Ping) {
  auto& client = GetTransactionClient();

  EXPECT_NO_THROW(Get(client->Ping(0)));
  EXPECT_EQ(Get(client->PingMessage(0, "Ping")), "Ping");
}

// Publish test

UTEST_F(RedisClientTransactionTest, Rename) {
  auto& client = GetTransactionClient();

  Get(client->Set("key", "value"));
  EXPECT_NO_THROW(Get(client->Rename("key", "new key")));
  auto result = Get(client->Get("new key"));
  EXPECT_TRUE(result.has_value());
}

UTEST_F(RedisClientTransactionTest, Rpop) {
  auto& client = GetTransactionClient();

  Get(client->Rpush("list", std::vector<std::string>{"1", "2"}));
  auto result = Get(client->Rpop("list"));
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "2");
  Get(client->Rpop("list"));
  result = Get(client->Rpop("list"));
  EXPECT_FALSE(result.has_value());
}

UTEST_F(RedisClientTransactionTest, Rpush) {
  auto& client = GetTransactionClient();

  EXPECT_EQ(Get(client->Rpush("list", std::vector<std::string>{"1", "2"})), 2);
  EXPECT_EQ(Get(client->Rpush("list", "3")), 3);
  auto result = Get(client->Lrange("list", 0, -1));
  EXPECT_EQ(result.size(), 3);
}

UTEST_F(RedisClientTransactionTest, Rpushx) {
  auto& client = GetTransactionClient();

  Get(client->Rpush("list", "1"));
  EXPECT_EQ(Get(client->Rpushx("list", "2")), 2);
  EXPECT_EQ(Get(client->Rpushx("other list", "1")), 0);
}

UTEST_F(RedisClientTransactionTest, Sadd) {
  auto& client = GetTransactionClient();

  EXPECT_EQ(Get(client->Sadd("set", "hello")), 1);
  EXPECT_EQ(
      Get(client->Sadd("set", std::vector<std::string>{"world", "world"})), 1);
  auto result = Get(client->Smembers("set"));
  EXPECT_EQ(result.size(), 2);
}

UTEST_F(RedisClientTransactionTest, Scard) {
  auto& client = GetTransactionClient();

  Get(client->Sadd("set", "hello"));
  Get(client->Sadd("set", "set"));
  EXPECT_EQ(Get(client->Scard("set")), 2);
}

UTEST_F(RedisClientTransactionTest, Set) {
  auto& client = GetTransactionClient();

  EXPECT_NO_THROW(Get(client->Set("key", "value")));
  auto result = Get(client->Get("key"));
  EXPECT_TRUE(result.has_value());
  EXPECT_NO_THROW(Get(
      client->Set("another key", "value", std::chrono::milliseconds{1999})));
  EXPECT_TRUE(Get(client->Ttl("another key")).KeyHasExpiration());

  EXPECT_TRUE(Get(client->SetIfExist("key", "new value")));
  EXPECT_FALSE(Get(client->SetIfExist("new key", "value")));
  EXPECT_FALSE(Get(client->SetIfNotExist("key", "new value")));
  EXPECT_TRUE(Get(client->SetIfNotExist("new key", "value")));
}

UTEST_F(RedisClientTransactionTest, Setex) {
  auto& client = GetTransactionClient();

  EXPECT_NO_THROW(Get(client->Setex("key", std::chrono::seconds{10}, "value")));
  EXPECT_EQ(Get(client->Ttl("key")).GetExpireSeconds(), 10);
}

UTEST_F(RedisClientTransactionTest, Sismember) {
  auto& client = GetTransactionClient();

  Get(client->Sadd("key", "one"));
  EXPECT_EQ(Get(client->Sismember("key", "one")), 1);
  EXPECT_EQ(Get(client->Sismember("key", "two")), 0);
}

UTEST_F(RedisClientTransactionTest, Smembers) {
  auto& client = GetTransactionClient();

  Get(client->Sadd("set", std::vector<std::string>{"world", "world", "hello"}));
  auto result = Get(client->Smembers("set"));
  EXPECT_EQ(result.size(), 2);
}

UTEST_F(RedisClientTransactionTest, Srandmember) {
  auto& client = GetTransactionClient();

  Get(client->Sadd("set", std::vector<std::string>{"one", "two", "three"}));
  auto result1 = Get(client->Srandmember("set"));
  EXPECT_TRUE(result1.has_value());
  EXPECT_TRUE(result1 == "one" || result1 == "two" || result1 == "three");

  auto result2 = Get(client->Srandmembers("set", 2));
  EXPECT_EQ(result2.size(), 2);
  result2 = Get(client->Srandmembers("set", -5));
  EXPECT_EQ(result2.size(), 5);
}

UTEST_F(RedisClientTransactionTest, Srem) {
  auto& client = GetTransactionClient();

  Get(client->Sadd("set",
                   std::vector<std::string>{"one", "two", "three", "four"}));
  EXPECT_EQ(Get(client->Srem("set", "one")), 1);

  EXPECT_EQ(Get(client->Srem("set",
                             std::vector<std::string>{"two", "three", "five"})),
            2);
}

UTEST_F(RedisClientTransactionTest, Strlen) {
  auto& client = GetTransactionClient();

  Get(client->Set("key", "value"));
  EXPECT_EQ(Get(client->Strlen("key")), 5);
  EXPECT_EQ(Get(client->Strlen("nonexisting")), 0);
}

UTEST_F(RedisClientTransactionTest, Time) {
  auto& client = GetTransactionClient();

  Get(client->Time(0));
}

UTEST_F(RedisClientTransactionTest, Type) {
  auto& client = GetTransactionClient();

  Get(client->Set("key1", "value"));
  Get(client->Lpush("key2", "value"));
  EXPECT_EQ(Get(client->Type("key1")), storages::redis::KeyType::kString);
  EXPECT_EQ(Get(client->Type("key2")), storages::redis::KeyType::kList);
}

UTEST_F(RedisClientTransactionTest, Zadd) {
  auto& client = GetTransactionClient();

  EXPECT_EQ(Get(client->Zadd("zset", 1., "one")), 1);
  storages::redis::ZaddOptions options;
  options.exist = storages::redis::ZaddOptions::Exist::kAddIfNotExist;
  EXPECT_EQ(Get(client->Zadd("zset", 2., "two", options)), 1);

  EXPECT_EQ(Get(client->Zadd("zset", {{2., "two"}, {3., "three"}})), 1);

  options.exist = storages::redis::ZaddOptions::Exist::kAddIfExist;
  options.return_value =
      storages::redis::ZaddOptions::ReturnValue::kChangedCount;
  EXPECT_EQ(Get(client->Zadd("zset", {{2.5, "two"}, {3.5, "three"}}, options)),
            2);
  auto result = Get(client->ZrangeWithScores("zset", 0, -1));
  EXPECT_EQ(result[1].score, 2.5);

  EXPECT_EQ(Get(client->ZaddIncr("zset", 1.2, "two")), 3.7);
  EXPECT_EQ(Get(client->ZaddIncr("zset", 4., "four")), 4.);
  EXPECT_EQ(Get(client->ZaddIncrExisting("zset", 1.1, "four")), 5.1);
  EXPECT_FALSE(Get(client->ZaddIncrExisting("zset", 1.1, "five")).has_value());
}

UTEST_F(RedisClientTransactionTest, Zcard) {
  auto& client = GetTransactionClient();

  Get(client->Zadd("zset", {{2., "two"}, {3., "three"}}));
  EXPECT_EQ(Get(client->Zcard("zset")), 2);
}

UTEST_F(RedisClientTransactionTest, Zrange) {
  auto& client = GetTransactionClient();

  Get(client->Zadd("zset", {{2., "two"}, {3., "three"}, {1., "one"}}));
  auto result = Get(client->Zrange("zset", 0, -1));
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], "one");
  auto result_with_scores = Get(client->ZrangeWithScores("zset", 0, -1));
  EXPECT_EQ(result_with_scores.size(), 3);
  EXPECT_EQ(result_with_scores[0].score, 1.);
}

UTEST_F(RedisClientTransactionTest, Zrangebyscore) {
  auto& client = GetTransactionClient();

  Get(client->Zadd(
      "zset",
      {{2., "two"}, {3., "three"}, {1., "one"}, {4., "four"}, {5., "five"}}));

  storages::redis::RangeOptions options;
  options.offset = 1;
  options.count = 10;

  auto result1 = Get(client->Zrangebyscore("zset", 2., 4.));
  EXPECT_EQ(result1.size(), 3);
  EXPECT_EQ(result1[0], "two");
  auto result2 = Get(client->Zrangebyscore("zset", 2., 4., options));
  EXPECT_EQ(result2.size(), 2);
  EXPECT_EQ(result2[0], "three");

  auto result1_scores = Get(client->ZrangebyscoreWithScores("zset", 2., 4.));
  EXPECT_EQ(result1_scores.size(), 3);
  EXPECT_EQ(result1_scores[0].member, "two");
  auto result2_scores =
      Get(client->ZrangebyscoreWithScores("zset", 2., 4., options));
  EXPECT_EQ(result2_scores.size(), 2);
  EXPECT_EQ(result2_scores[0].member, "three");
}

UTEST_F(RedisClientTransactionTest, ZrangebyscoreString) {
  auto& client = GetTransactionClient();

  Get(client->Zadd(
      "zset",
      {{2., "two"}, {3., "three"}, {1., "one"}, {4., "four"}, {5., "five"}}));

  storages::redis::RangeOptions options;
  options.offset = 1;
  options.count = 10;

  auto result1 = Get(client->Zrangebyscore("zset", "(2.0", "4.0"));
  EXPECT_EQ(result1.size(), 2);
  EXPECT_EQ(result1[0], "three");
  auto result2 = Get(client->Zrangebyscore("zset", "2.0", "(5.0", options));
  EXPECT_EQ(result2.size(), 2);
  EXPECT_EQ(result2[0], "three");
  auto result1_scores =
      Get(client->ZrangebyscoreWithScores("zset", "(2.0", "4.0"));
  EXPECT_EQ(result1_scores.size(), 2);
  EXPECT_EQ(result1_scores[0].member, "three");
  auto result2_scores =
      Get(client->ZrangebyscoreWithScores("zset", "2.0", "(5.0", options));
  EXPECT_EQ(result2_scores.size(), 2);
  EXPECT_EQ(result2_scores[0].member, "three");
}

UTEST_F(RedisClientTransactionTest, Zrem) {
  auto& client = GetTransactionClient();

  Get(client->Zadd(
      "zset",
      {{2., "two"}, {3., "three"}, {1., "one"}, {4., "four"}, {5., "five"}}));
  EXPECT_EQ(Get(client->Zrem("zset", "two")), 1);
  EXPECT_EQ(Get(client->Zrem("zset",
                             std::vector<std::string>{"two", "one", "three"})),
            2);
  auto result = Get(client->Zrange("zset", 0, -1));
  EXPECT_EQ(result.size(), 2);
}

UTEST_F(RedisClientTransactionTest, Zremrangebyrank) {
  auto& client = GetTransactionClient();

  Get(client->Zadd("zset", {{2., "two"}, {3., "three"}, {1., "one"}}));
  EXPECT_EQ(Get(client->Zremrangebyrank("zset", 0, 1)), 2);
  auto result = Get(client->Zrange("zset", 0, -1));
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], "three");
}

UTEST_F(RedisClientTransactionTest, Zremrangebyscore) {
  auto& client = GetTransactionClient();

  Get(client->Zadd("zset", {{2., "two"}, {3., "three"}, {1., "one"}}));
  EXPECT_EQ(Get(client->Zremrangebyscore("zset", 1., 2.)), 2);
  auto result = Get(client->Zrange("zset", 0, -1));
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], "three");

  Get(client->Zadd("zset", {{2., "two"}, {3., "three"}, {1., "one"}}));
  EXPECT_EQ(Get(client->Zremrangebyscore("zset", "-inf", "(3")), 2);
  result = Get(client->Zrange("zset", 0, -1));
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], "three");
}

UTEST_F(RedisClientTransactionTest, Zscore) {
  auto& client = GetTransactionClient();

  Get(client->Zadd("zset", 2., "two"));
  EXPECT_EQ(Get(client->Zscore("zset", "two")), 2.);
}

namespace {

std::uint64_t GetCommandCount(
    std::map<std::string, redis::ShardStatistics>& stat) {
  std::uint64_t command_count = 0;
  std::for_each(stat.begin(), stat.end(), [&](auto& shard) {
    command_count +=
        shard.second.shard_total
            .error_count[static_cast<std::size_t>(redis::ReplyStatus::kOk)];
  });
  return command_count;
}

}  // namespace

UTEST_F(RedisClientTransactionTest, NotReadOnlySetSet) {
  auto client = GetClient();
  auto sentinel = GetSentinel();

  auto before = sentinel->GetStatistics({});

  auto transaction = client->Multi();
  auto set0 = transaction->Set("key", "value");
  auto set1 = transaction->Set("key", "value");
  transaction->Exec(kDefaultCc).Get();

  auto after = sentinel->GetStatistics({});
  std::uint64_t master_command_count =
      GetCommandCount(after.masters) - GetCommandCount(before.masters);
  std::uint64_t slave_command_count =
      GetCommandCount(after.slaves) - GetCommandCount(before.slaves);

  EXPECT_EQ(master_command_count, 4);
  EXPECT_EQ(slave_command_count, 0);
}

UTEST_F(RedisClientTransactionTest, NotReadOnlySetGet) {
  auto client = GetClient();
  auto sentinel = GetSentinel();

  auto before = sentinel->GetStatistics({});

  {
    auto transaction = client->Multi();
    auto set = transaction->Set("key", "value");
    auto get = transaction->Get("key");
    transaction->Exec(kDefaultCc).Get();
  }
  {
    auto transaction = client->Multi();
    auto get = transaction->Get("key");
    auto set = transaction->Set("key", "value");
    transaction->Exec(kDefaultCc).Get();
  }

  auto after = sentinel->GetStatistics({});
  std::uint64_t master_command_count =
      GetCommandCount(after.masters) - GetCommandCount(before.masters);
  std::uint64_t slave_command_count =
      GetCommandCount(after.slaves) - GetCommandCount(before.slaves);

  EXPECT_EQ(master_command_count, 8);
  EXPECT_EQ(slave_command_count, 0);
}

UTEST_F(RedisClientTransactionTest, ReadOnlyGetGet) {
  auto client = GetClient();
  auto sentinel = GetSentinel();

  auto before = sentinel->GetStatistics({});

  auto transaction = client->Multi();
  auto get0 = transaction->Get("key");
  auto get1 = transaction->Get("key");
  transaction->Exec(kDefaultCc).Get();

  auto after = sentinel->GetStatistics({});
  std::uint64_t master_command_count =
      GetCommandCount(after.masters) - GetCommandCount(before.masters);
  std::uint64_t slave_command_count =
      GetCommandCount(after.slaves) - GetCommandCount(before.slaves);

  EXPECT_EQ(master_command_count, 0);
  EXPECT_EQ(slave_command_count, 4);
}

USERVER_NAMESPACE_END
