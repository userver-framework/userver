#include <userver/utest/utest.hpp>

#include <atomic>
#include <memory>

#include <storages/redis/client_impl.hpp>
#include <storages/redis/impl/subscribe_sentinel.hpp>
#include <storages/redis/util_redistest.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utils/text.hpp>

#include <userver/storages/redis/impl/reply.hpp>
#include <userver/storages/redis/impl/sentinel.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

auto RequestKeys(redis::Sentinel& sentinel, redis::CommandControl cc = {}) {
  return sentinel.MakeRequest({"keys", "*"}, 0, false,
                              sentinel.GetCommandControl(cc));
}

}  // namespace

UTEST(Sentinel, ReplyServerId) {
  /* TODO: hack! sentinel is too slow to learn new replicaset members :-( */
  engine::SleepFor(std::chrono::milliseconds(11000));

  auto thread_pools = std::make_shared<redis::ThreadPools>(
      redis::kDefaultSentinelThreadPoolSize,
      redis::kDefaultRedisThreadPoolSize);
  auto sentinel = redis::Sentinel::CreateSentinel(
      thread_pools, GetTestsuiteRedisSettings(), "none", "pub",
      redis::KeyShardFactory{""});
  sentinel->WaitConnectedDebug();

  auto req = RequestKeys(*sentinel);
  auto reply = req.Get();
  auto first_id = reply->server_id;
  ASSERT_FALSE(first_id.IsAny());

  // We want at least 1 id != first_id
  const auto max_i = 10;
  bool has_any_distinct_id = false;
  for (int i = 0; i < max_i; i++) {
    auto req = RequestKeys(*sentinel);
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
      redis::KeyShardFactory{""});
  sentinel->WaitConnectedDebug();

  auto req = RequestKeys(*sentinel);
  auto reply = req.Get();
  auto first_id = reply->server_id;
  EXPECT_FALSE(first_id.IsAny());

  const auto max_i = 10;
  for (int i = 0; i < max_i; i++) {
    redis::CommandControl cc;
    cc.force_server_id = first_id;

    auto req = RequestKeys(*sentinel, cc);
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
      redis::KeyShardFactory{""});
  sentinel->WaitConnectedDebug();

  // w/o force_server_id
  auto req1 = RequestKeys(*sentinel);
  auto reply1 = req1.Get();
  EXPECT_TRUE(reply1->IsOk());

  // w force_server_id
  redis::CommandControl cc;
  cc.force_server_id = redis::ServerId::Invalid();
  auto req2 = RequestKeys(*sentinel, cc);
  auto reply2 = req2.Get();

  EXPECT_FALSE(reply2->IsOk());
}

UTEST(Sentinel, MasterShutdownAndFailover) {
  auto thread_pools = std::make_shared<redis::ThreadPools>(
      redis::kDefaultSentinelThreadPoolSize,
      redis::kDefaultRedisThreadPoolSize);

  auto sentinel = redis::Sentinel::CreateSentinel(
      thread_pools, GetTestsuiteRedisSettings(), "none", "pub",
      redis::KeyShardFactory{""});
  sentinel->WaitConnectedDebug();

  auto subscribe_sentinel = redis::SubscribeSentinel::Create(
      thread_pools, GetTestsuiteRedisSettings(), "none", "pub", false, {});
  subscribe_sentinel->WaitConnectedDebug();

  std::atomic_int sentinel_epoch{0};
  std::atomic_int master_epoch{0};

  // subscribe to configuration pub/sub channel to receive config epochs values
  auto token = subscribe_sentinel->Subscribe(
      "__sentinel__:hello",
      [&](const std::string& /*channel*/, const std::string& message) {
        // message is composed of 8 comma-separated tokens:
        // sentinel_ip,sentinel_port,run_id,sentinel_config_epoch,
        // master_name,master_ip,master_port,master_config_epoch
        auto parts = utils::text::Split(message, ",");
        master_epoch = std::stoi(parts.rbegin()[0]);
        sentinel_epoch = std::stoi(parts.rbegin()[4]);
      });

  auto client = std::make_shared<storages::redis::ClientImpl>(sentinel);

  ASSERT_NO_THROW(client->Set("key0", "foo1", {}).Get());
  EXPECT_EQ(client->Get("key0", {}).Get(), "foo1");

  // send shutdown command to current master
  sentinel->AsyncCommand(
      redis::PrepareCommand(redis::CmdArgs{"SHUTDOWN", "NOSAVE"},
                            [&](const auto& /*cmd*/, auto reply) {
                              EXPECT_EQ(reply->data.GetType(),
                                        redis::ReplyData::Type::kNoReply);
                            }),
      true);

  // command sentinel to perform a manual failover
  sentinel->AsyncCommandToSentinel(redis::PrepareCommand(
      redis::CmdArgs{"SENTINEL", "FAILOVER", "test_master0"},
      [&](const auto& /*cmd*/, auto reply) {
        EXPECT_FALSE(reply->data.IsError()) << reply->data.ToDebugString();
      }));

  // successful failover should increment both config epoch values
  while (master_epoch == 0) engine::SleepFor(std::chrono::milliseconds(100));
  EXPECT_EQ(sentinel_epoch, 1);
  EXPECT_EQ(master_epoch, 1);

  // learning about a new master will take some time
  auto cc = redis::CommandControl{std::chrono::milliseconds{100},
                                  std::chrono::milliseconds{10000}, 100};
  ASSERT_NO_THROW(client->Set("key0", "foo2", cc).Get());
  EXPECT_EQ(client->Get("key0", cc).Get(), "foo2");
}

USERVER_NAMESPACE_END
