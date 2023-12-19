#include <userver/utest/utest.hpp>

#include <thread>

#include <userver/engine/sleep.hpp>

#include "server_common_sentinel_test.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

// 100ms should be enough, but valgrind is too slow
constexpr auto kSentinelChangeHostsWaitingTime = std::chrono::milliseconds(500);
constexpr auto kSentinelChangeHostsMaxAttempts = 10;

auto MakeGetRequest(redis::Sentinel& sentinel, const std::string& key,
                    redis::CommandControl cc = {}) {
  return sentinel.MakeRequest({"get", key}, key, false,
                              sentinel.GetCommandControl(cc));
}

bool CheckMasterChanged(redis::Sentinel& sentinel, const size_t expected_idx,
                        int magic_value) {
  for (auto i = 0; i < kSentinelChangeHostsMaxAttempts; ++i) {
    auto res = MakeGetRequest(sentinel, "value").Get();
    LOG_DEBUG() << "got reply with type=" << res->data.GetTypeString()
                << " data=" << res->data.ToDebugString();
    if (static_cast<size_t>(res->data.GetInt() - magic_value) == expected_idx)
      return true;
    engine::SleepFor(kSentinelChangeHostsWaitingTime);
  }
  return false;
}

}  // namespace

UTEST(Redis, SentinelSingleMaster) {
  const size_t master_count = 1;
  const size_t slave_count = 0;
  const size_t sentinel_count = 1;
  const int magic_value = 2;

  SentinelTest sentinel_test(sentinel_count, master_count, slave_count,
                             magic_value);
  auto& sentinel = sentinel_test.SentinelClient();

  EXPECT_TRUE(sentinel_test.Master().WaitForFirstPingReply(kSmallPeriod));

  auto res = MakeGetRequest(sentinel, "value").Get();
  ASSERT_TRUE(res->data.IsInt());
  EXPECT_TRUE(res->data.GetInt() == magic_value);
}

UTEST(Redis, SentinelMastersChanging) {
  const size_t master_count = 3;
  const size_t slave_count = 0;
  const size_t sentinel_count = 1;
  const int magic_value_add = 98;
  size_t master_idx{0};

  SentinelTest sentinel_test(sentinel_count, master_count, slave_count,
                             magic_value_add);
  auto& sentinel = sentinel_test.SentinelClient();

  for (size_t i = 0; i < master_count; i++) {
    if (master_idx != i) {
      master_idx = i;
      auto masters_handler =
          sentinel_test.Sentinel().RegisterSentinelMastersHandler(
              {{sentinel_test.RedisName(), kLocalhost,
                sentinel_test.Master(master_idx).GetPort()}});
      sentinel.ForceUpdateHosts();
      EXPECT_TRUE(masters_handler->WaitForFirstReply(kSmallPeriod));
      EXPECT_TRUE(sentinel_test.Master(master_idx)
                      .WaitForFirstPingReply(kSentinelChangeHostsWaitingTime));
    }

    ASSERT_TRUE(CheckMasterChanged(sentinel, master_idx, magic_value_add));
  }
}

UTEST(Redis, SentinelMastersChangingErrors) {
  const size_t master_count = 5;
  const size_t slave_count = 0;
  const size_t sentinel_count = 3;
  const size_t bad_redis_idx = 3;
  const int magic_value_add = 132;
  const size_t redis_thread_count = 3;
  size_t master_idx{0};

  SentinelTest sentinel_test(sentinel_count, master_count, slave_count,
                             magic_value_add, 0, redis_thread_count);
  auto& sentinel = sentinel_test.SentinelClient();

  size_t expected_redis_idx_reply_from = master_idx;
  for (size_t i = 0; i < master_count; i++) {
    if (master_idx != i) {
      master_idx = i;
      std::vector<MockRedisServer::HandlerPtr> masters_handlers;
      for (size_t sentinel_idx = 0; sentinel_idx < sentinel_count;
           sentinel_idx++) {
        size_t quorum = sentinel_count / 2 + 1;
        if (master_idx == bad_redis_idx && sentinel_idx < quorum) {
          masters_handlers.push_back(
              sentinel_test.Sentinel(sentinel_idx)
                  .RegisterErrorReplyHandler(
                      "SENTINEL", {"MASTERS"},
                      "some incorrect SENTINEL MASTERS reply"));
        } else {
          masters_handlers.push_back(
              sentinel_test.Sentinel(sentinel_idx)
                  .RegisterSentinelMastersHandler(
                      {{sentinel_test.RedisName(), kLocalhost,
                        sentinel_test.Master(master_idx).GetPort()}}));
        }
      }
      sentinel.ForceUpdateHosts();
      for (auto& handler : masters_handlers) {
        EXPECT_TRUE(handler->WaitForFirstReply(kSmallPeriod));
      }
      if (master_idx == bad_redis_idx) {
        EXPECT_FALSE(
            sentinel_test.Master(master_idx)
                .WaitForFirstPingReply(kSentinelChangeHostsWaitingTime));
      } else {
        EXPECT_TRUE(
            sentinel_test.Master(master_idx)
                .WaitForFirstPingReply(kSentinelChangeHostsWaitingTime));
        expected_redis_idx_reply_from = master_idx;
      }
    }

    ASSERT_TRUE(CheckMasterChanged(sentinel, expected_redis_idx_reply_from,
                                   magic_value_add));
  }
}

UTEST(Redis, SentinelMasterAndSlave) {
  const size_t master_count = 1;
  const size_t slave_count = 1;
  const size_t sentinel_count = 1;
  const int magic_value_master = 238;
  const int magic_value_slave = -238;

  SentinelTest sentinel_test(sentinel_count, master_count, slave_count,
                             magic_value_master, magic_value_slave);
  auto& sentinel = sentinel_test.SentinelClient();

  EXPECT_TRUE(sentinel_test.Master().WaitForFirstPingReply(kSmallPeriod));
  EXPECT_TRUE(sentinel_test.Slave().WaitForFirstPingReply(kSmallPeriod));

  {
    auto res = MakeGetRequest(sentinel, "value").Get();
    ASSERT_TRUE(res->data.IsInt());
    EXPECT_TRUE(res->data.GetInt() == magic_value_slave);
  }

  {
    redis::CommandControl force_master_cc;
    force_master_cc.force_request_to_master = true;
    auto res = MakeGetRequest(sentinel, "value", force_master_cc).Get();
    ASSERT_TRUE(res->data.IsInt());
    EXPECT_TRUE(res->data.GetInt() == magic_value_master);
  }
}

UTEST(Redis, SentinelCcRetryToMasterOnNilReply) {
  const size_t master_count = 1;
  const size_t slave_count = 1;
  const size_t sentinel_count = 1;
  const int magic_value_master = 238;

  SentinelTest sentinel_test(sentinel_count, master_count, slave_count,
                             magic_value_master);
  auto& sentinel = sentinel_test.SentinelClient();

  EXPECT_TRUE(sentinel_test.Master().WaitForFirstPingReply(kSmallPeriod));
  EXPECT_TRUE(sentinel_test.Slave().WaitForFirstPingReply(kSmallPeriod));

  sentinel_test.Slave().RegisterNilReplyHandler("GET");

  {
    auto res = MakeGetRequest(sentinel, "slave_nil").Get();
    EXPECT_TRUE(res->data.IsNil());
  }
  {
    redis::CommandControl force_master_cc;
    force_master_cc.max_retries = 1;
    force_master_cc.force_request_to_master = true;
    auto res = MakeGetRequest(sentinel, "slave_nil", force_master_cc).Get();
    EXPECT_TRUE(res->data.IsInt());
    EXPECT_TRUE(res->data.GetInt() == magic_value_master);
  }
  {
    redis::CommandControl no_force_master_cc;
    no_force_master_cc.max_retries = 2;
    no_force_master_cc.force_request_to_master = false;
    auto res = MakeGetRequest(sentinel, "slave_nil", no_force_master_cc).Get();
    EXPECT_TRUE(res->data.IsNil());
  }
  {
    redis::CommandControl cc;
    cc.max_retries = 1;
    cc.force_retries_to_master_on_nil_reply = true;
    auto res = MakeGetRequest(sentinel, "slave_nil", cc).Get();
    EXPECT_TRUE(res->data.IsNil());
  }
  {
    redis::CommandControl cc;
    cc.max_retries = 2;
    cc.force_retries_to_master_on_nil_reply = true;
    auto res = MakeGetRequest(sentinel, "slave_nil", cc).Get();
    EXPECT_TRUE(res->data.IsInt());
    EXPECT_TRUE(res->data.GetInt() == magic_value_master);
  }
  {
    redis::CommandControl cc;
    cc.max_retries = 2;
    auto res = MakeGetRequest(sentinel, "slave_nil", cc).Get();
    EXPECT_TRUE(res->data.IsNil());
  }

  auto slave_nil_handler =
      sentinel_test.Slave().RegisterNilReplyHandler("GET", {"master_nil"});
  auto master_nil_handler =
      sentinel_test.Master().RegisterNilReplyHandler("GET", {"master_nil"});
  {
    redis::CommandControl cc;
    cc.max_retries = 5;
    cc.force_retries_to_master_on_nil_reply = true;
    auto res = MakeGetRequest(sentinel, "master_nil", cc).Get();
    EXPECT_TRUE(res->data.IsNil());
    EXPECT_EQ(slave_nil_handler->GetReplyCount(), 1UL);
    EXPECT_EQ(master_nil_handler->GetReplyCount(), 1UL);
  }
}

UTEST(Redis, SentinelForceShardIdx) {
  const size_t shard_count = 3;
  const size_t sentinel_count = 1;
  const int magic_value_add = 98;

  SentinelShardTest sentinel_test(sentinel_count, shard_count, magic_value_add,
                                  magic_value_add);
  auto& sentinel = sentinel_test.SentinelClient();

  for (size_t shard_idx = 0; shard_idx < shard_count; shard_idx++) {
    EXPECT_TRUE(sentinel_test.Master(shard_idx).WaitForFirstPingReply(
        kSentinelChangeHostsWaitingTime))
        << "shard_idx=" << shard_idx;
    redis::CommandControl cc;
    cc.force_shard_idx = shard_idx;
    auto res = MakeGetRequest(sentinel, "value", cc).Get();
    LOG_DEBUG() << "got reply with type=" << res->data.GetTypeString()
                << " data=" << res->data.ToDebugString();
    ASSERT_TRUE(res->data.IsInt());
    EXPECT_EQ(shard_idx,
              static_cast<size_t>(res->data.GetInt() - magic_value_add))
        << " shard_idx=" << shard_idx;
  }
}

USERVER_NAMESPACE_END
