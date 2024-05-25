#include "cluster_subscription_storage.hpp"
#include "sentinel.hpp"
#include "subscription_storage.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <unordered_map>
#include "command.hpp"
#include "userver/storages/redis/command_control.hpp"
#include "userver/storages/redis/impl/reply.hpp"

USERVER_NAMESPACE_BEGIN

namespace {
redis::ServerId MakeServerId(std::string description) {
  auto ret = redis::ServerId::Generate();
  ret.SetDescription(std::move(description));
  return ret;
}

class SubscriptionTest : public ::testing::Test {
 public:
  static void SetUpTestSuite() {}

  static void TearDownTestSuite() {}

  void SetUp() override {
    storage_ =
        std::make_shared<redis::ClusterSubscriptionStorage>(kShardsCount);
    auto sharded_subscribe_callback = [&](const std::string& /*channel*/,
                                          redis::CommandPtr cmd) {
      ASSERT_TRUE(cmd->control.force_server_id);
      const auto& host = cmd->control.force_server_id->GetDescription();
      cmds_.push_back(cmd);
      if (host.size()) ssubscriptions_by_host_[host]++;
    };
    auto sharded_unsubscribe_callback = [&](const std::string& /*channel*/,
                                            redis::CommandPtr cmd) {
      ASSERT_TRUE(cmd->control.force_server_id);
      const auto& host = cmd->control.force_server_id->GetDescription();
      if (host.size()) ssubscriptions_by_host_[host]--;
    };

    auto subscribe_callback = [&](size_t /*shard_idx*/, redis::CommandPtr cmd) {
      ASSERT_TRUE(cmd->control.force_server_id);
      const auto& host = cmd->control.force_server_id->GetDescription();

      cmds_.push_back(cmd);

      if (host.size()) subscriptions_by_host_[host]++;
    };
    auto unsubscribe_callback = [&](size_t /*shard_idx*/,
                                    redis::CommandPtr cmd) {
      ASSERT_TRUE(cmd->control.force_server_id);
      const auto& host = cmd->control.force_server_id->GetDescription();
      if (host.size()) subscriptions_by_host_[host]--;
    };
    storage_->SetSubscribeCallback(subscribe_callback);
    storage_->SetUnsubscribeCallback(unsubscribe_callback);
    storage_->SetShardedSubscribeCallback(sharded_subscribe_callback);
    storage_->SetShardedUnsubscribeCallback(sharded_unsubscribe_callback);
  }

  void Subscribe(const std::string& channel_name) {
    auto message_callback = [](const std::string& /*channel*/,
                               const std::string& /*message*/) {
      return redis::Sentinel::Outcome::kOk;
    };
    auto token = storage_->Subscribe(channel_name, message_callback, {});
    tokens_.push_back(std::move(token));
  }
  void Ssubscribe(const std::string& channel_name) {
    auto message_callback = [](const std::string& /*channel*/,
                               const std::string& /*message*/) {
      return redis::Sentinel::Outcome::kOk;
    };
    auto token = storage_->Ssubscribe(channel_name, message_callback, {});
    tokens_.push_back(std::move(token));
  }
  void ProcessCommands() {
    for (auto& cmd : cmds_) {
      const auto& command = cmd->args.args[0][0];
      const auto& channel = cmd->args.args[0][1];
      redis::ReplyData reply_data(redis::ReplyData::Array{
          redis::ReplyData(command), redis::ReplyData(channel),
          redis::ReplyData(1)});
      redis::ReplyPtr reply =
          std::make_shared<redis::Reply>(command, std::move(reply_data));
      reply->server_id = server_ids[0];
      cmd->callback({}, reply);
    }
    cmds_.clear();
  }

  void Rebalance(size_t shard) { storage_->DoRebalance(shard, weights); }

  const auto& GetSubscriptionsByHost() const { return subscriptions_by_host_; }
  const auto& GetShardedSubscriptionsByHost() const {
    return ssubscriptions_by_host_;
  }

 private:
  const std::vector<redis::ServerId> server_ids = std::vector{
      MakeServerId("host0"), MakeServerId("host1"), MakeServerId("host2"),
      MakeServerId("host3"), MakeServerId("host4"), MakeServerId("host5"),
  };

  /// Which hosts do we have
  const redis::ClusterSubscriptionStorage::ServerWeights weights = {
      {server_ids[0], 1}, {server_ids[1], 1}, {server_ids[2], 1},
      {server_ids[3], 1}, {server_ids[4], 1}, {server_ids[5], 1},

  };

  const size_t kShardsCount = 3;
  std::shared_ptr<redis::ClusterSubscriptionStorage> storage_;
  std::unordered_map<std::string, size_t> subscriptions_by_host_;
  std::unordered_map<std::string, size_t> ssubscriptions_by_host_;
  std::vector<redis::SubscriptionToken> tokens_;
  std::vector<redis::CommandPtr> cmds_;
};

}  // namespace

/// Test subscriptions are evenly distributed between connections
TEST_F(SubscriptionTest, Base) {
  const std::unordered_map<std::string, size_t> expected = {
      /// {"host0", 1}, - no need to resubscribe host0  because it should be
      /// already have enough subscriptions.
      /// Initial subscription done to host0 (see cmds processing)
      {"host1", 1}, {"host2", 1}, {"host3", 1}, {"host4", 1}, {"host5", 1},
  };

  Subscribe("channel0");
  Subscribe("channel1");
  Subscribe("channel2");
  Subscribe("channel3");
  Subscribe("channel4");
  Subscribe("channel5");

  ProcessCommands();
  Rebalance(0);

  const auto& subscriptions_by_host = GetSubscriptionsByHost();

  /// Check balance
  EXPECT_EQ(5ull, subscriptions_by_host.size());
  EXPECT_EQ(expected, subscriptions_by_host);
}

/// Test subscriptions are evenly distributed between connections
TEST_F(SubscriptionTest, Sharded) {
  const std::unordered_map<std::string, size_t> expected = {
      /// {"host0", 1}, - no need to resubscribe host0  because it should be
      /// already have enough subscriptions.
      /// Initial subscription done to host0 (see ProcessCommands())
      {"host1", 1}, {"host2", 1}, {"host3", 1}, {"host4", 1}, {"host5", 1},
  };

  Ssubscribe("channel0");
  Ssubscribe("channel1");
  Ssubscribe("channel2");
  Ssubscribe("channel3");
  Ssubscribe("channel4");
  Ssubscribe("channel5");

  ProcessCommands();
  Rebalance(0);

  const auto& subscriptions_by_host = GetShardedSubscriptionsByHost();

  /// Check balance
  EXPECT_EQ(5ull, subscriptions_by_host.size());
  EXPECT_EQ(expected, subscriptions_by_host);
}

USERVER_NAMESPACE_END
