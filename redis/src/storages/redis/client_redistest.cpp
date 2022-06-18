#include <userver/utest/utest.hpp>

#include <memory>
#include <string>

#include <engine/task/task_context.hpp>
#include <storages/redis/client_impl.hpp>
#include <storages/redis/util_redistest.hpp>
#include <userver/storages/redis/impl/sentinel.hpp>
#include <userver/storages/redis/impl/thread_pools.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

std::shared_ptr<storages::redis::Client> GetClient() {
  auto thread_pools = std::make_shared<redis::ThreadPools>(
      redis::kDefaultSentinelThreadPoolSize,
      redis::kDefaultRedisThreadPoolSize);
  auto sentinel = redis::Sentinel::CreateSentinel(
      std::move(thread_pools), GetTestsuiteRedisSettings(), "none", "pub",
      redis::KeyShardFactory{""});
  sentinel->WaitConnectedDebug();

  return std::make_shared<storages::redis::ClientImpl>(std::move(sentinel));
}

/// [Sample Redis Client usage]
void RedisClientSampleUsage(storages::redis::Client& client) {
  client.Rpush("sample_list", "a", {}).Get();
  client.Rpush("sample_list", "b", {}).Get();
  const auto length = client.Llen("sample_list", {}).Get();

  EXPECT_EQ(length, 2);
}
/// [Sample Redis Client usage]

/// [Sample Redis Cancel request]
std::string RedisClientCancelRequest(storages::redis::Client& client) {
  auto result = client.Get("foo", {});

  engine::current_task::GetCurrentTaskContext().RequestCancel(
      engine::TaskCancellationReason::kUserRequest);

  // Throws redis::RequestCancelledException if Redis was not
  // fast enough to answer
  return result.Get();
}
/// [Sample Redis Cancel request]

}  // namespace

UTEST(RedisClient, Sample) { RedisClientSampleUsage(*GetClient()); }

UTEST(RedisClient, CancelRequest) {
  try {
    EXPECT_EQ("", RedisClientCancelRequest(*GetClient()));
  catch (const redis::RequestCancelledException&) {}
}

UTEST(RedisClient, Lrem) {
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

UTEST(RedisClient, Lpushx) {
  auto client = GetClient();
  // Missing array - does nothing
  EXPECT_EQ(client->Lpushx("pushx_testlist", "a", {}).Get(), 0);
  EXPECT_EQ(client->Rpushx("pushx_testlist", "a", {}).Get(), 0);
  // // Init list
  EXPECT_EQ(client->Lpush("pushx_testlist", "a", {}).Get(), 1);
  // // List exists - appends values
  EXPECT_EQ(client->Lpushx("pushx_testlist", "a", {}).Get(), 2);
  EXPECT_EQ(client->Rpushx("pushx_testlist", "a", {}).Get(), 3);
}

UTEST(RedisClient, SetGet) {
  auto client = GetClient();
  UEXPECT_NO_THROW(client->Set("key0", "foo", {}).Get());
  EXPECT_EQ(client->Get("key0", {}).Get(), "foo");
}

UTEST(RedisClient, Mget) {
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

USERVER_NAMESPACE_END
