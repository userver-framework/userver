#include <userver/utest/utest.hpp>

#include <memory>

#include <storages/redis/util_redistest.hpp>
#include <userver/engine/sleep.hpp>

#include <userver/storages/redis/impl/reply.hpp>
#include <userver/storages/redis/impl/sentinel.hpp>

UTEST(Sentinel, ReplyServerId) {
  /* TODO: hack! sentinel is too slow to learn new replicaset members :-( */
  engine::SleepFor(std::chrono::milliseconds(11000));

  auto thread_pools = std::make_shared<redis::ThreadPools>(
      redis::kDefaultSentinelThreadPoolSize,
      redis::kDefaultRedisThreadPoolSize);
  auto sentinel = redis::Sentinel::CreateSentinel(
      thread_pools, GetTestsuiteRedisSettings(), "none", "pub",
      redis::KeyShardFactory{""}, {});
  sentinel->WaitConnectedDebug();

  auto req = sentinel->Keys("*", 0);
  auto reply = req.Get();
  auto first_id = reply->server_id;
  ASSERT_FALSE(first_id.IsAny());

  // We want at least 1 id != first_id
  const auto max_i = 10;
  bool has_any_distinct_id = false;
  for (int i = 0; i < max_i; i++) {
    auto req = sentinel->Keys("*", 0);
    auto reply = req.Get();
    auto id = reply->server_id;
    if (!(id == first_id)) has_any_distinct_id = true;

    EXPECT_TRUE(reply->IsOk());
    EXPECT_FALSE(id.IsAny());
  }
  EXPECT_TRUE(has_any_distinct_id);
}

UTEST(Sentinel, ForceServerId) {
  auto thread_pools = std::make_shared<redis::ThreadPools>(
      redis::kDefaultSentinelThreadPoolSize,
      redis::kDefaultRedisThreadPoolSize);
  auto sentinel = redis::Sentinel::CreateSentinel(
      thread_pools, GetTestsuiteRedisSettings(), "none", "pub",
      redis::KeyShardFactory{""}, {});
  sentinel->WaitConnectedDebug();

  auto req = sentinel->Keys("*", 0);
  auto reply = req.Get();
  auto first_id = reply->server_id;
  EXPECT_FALSE(first_id.IsAny());

  const auto max_i = 10;
  for (int i = 0; i < max_i; i++) {
    redis::CommandControl cc;
    cc.force_server_id = first_id;

    auto req = sentinel->Keys("*", 0, cc);
    auto reply = req.Get();
    auto id = reply->server_id;

    EXPECT_TRUE(reply->IsOk());
    EXPECT_FALSE(id.IsAny());
    EXPECT_EQ(first_id, id);
  }
}

UTEST(Sentinel, ForceNonExistingServerId) {
  auto thread_pools = std::make_shared<redis::ThreadPools>(
      redis::kDefaultSentinelThreadPoolSize,
      redis::kDefaultRedisThreadPoolSize);
  auto sentinel = redis::Sentinel::CreateSentinel(
      thread_pools, GetTestsuiteRedisSettings(), "none", "pub",
      redis::KeyShardFactory{""}, {});
  sentinel->WaitConnectedDebug();

  // w/o force_server_id
  auto req1 = sentinel->Keys("*", 0);
  auto reply1 = req1.Get();
  EXPECT_TRUE(reply1->IsOk());

  // w force_server_id
  redis::CommandControl cc;
  cc.force_server_id = redis::ServerId::Invalid();
  auto req2 = sentinel->Keys("*", 0, cc);
  auto reply2 = req2.Get();

  EXPECT_FALSE(reply2->IsOk());
}
