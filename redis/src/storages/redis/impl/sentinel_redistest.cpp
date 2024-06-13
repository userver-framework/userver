#include <userver/utest/utest.hpp>

#include <userver/engine/sleep.hpp>
#include <userver/storages/redis/impl/reply.hpp>

#include <storages/redis/impl/command.hpp>
#include <storages/redis/utest/impl/redis_connection_state.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class SentinelTest : public ::testing::Test,
                     public storages::redis::utest::impl::RedisConnectionState {
 protected:
  auto RequestKeys(redis::CommandControl cc = {}) {
    return GetSentinel()->MakeRequest({"keys", "*"}, 0, false,
                                      GetSentinel()->GetCommandControl(cc));
  }
};

}  // namespace

UTEST_F(SentinelTest, ReplyServerId) {
  constexpr auto kTotalRequests = 5;
  constexpr auto kSleepInterval = std::chrono::seconds{1};
  // let's give sentinel enough time to learn about new replicas
  auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  std::set<redis::ServerId> server_ids;
  while (!deadline.IsReached()) {
    for (auto i = 0; i < kTotalRequests; ++i) {
      auto reply = RequestKeys().Get();
      auto id = reply->server_id;
      EXPECT_FALSE(id.IsAny());
      server_ids.insert(id);
    }
    if (server_ids.size() > 1) break;
    engine::SleepFor(kSleepInterval);
  }
  EXPECT_GT(server_ids.size(), 1);
}

UTEST_F(SentinelTest, ForceServerId) {
  auto reply = RequestKeys().Get();
  auto first_id = reply->server_id;
  EXPECT_FALSE(first_id.IsAny());

  const auto max_i = 10;
  for (int i = 0; i < max_i; i++) {
    redis::CommandControl cc;
    cc.force_server_id = first_id;

    auto reply = RequestKeys(cc).Get();
    auto id = reply->server_id;

    EXPECT_TRUE(reply->IsOk());
    EXPECT_FALSE(id.IsAny());
    EXPECT_EQ(first_id, id);
  }
}

UTEST_F(SentinelTest, ForceNonExistingServerId) {
  // w/o force_server_id
  auto reply1 = RequestKeys().Get();
  EXPECT_TRUE(reply1->IsOk());

  // w force_server_id
  redis::CommandControl cc;
  cc.force_server_id = redis::ServerId::Invalid();
  auto reply2 = RequestKeys(cc).Get();

  EXPECT_FALSE(reply2->IsOk());
}

USERVER_NAMESPACE_END
