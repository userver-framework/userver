#include "cluster_subscription_storage.hpp"
#include "subscription_storage.hpp"

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <engine/ev/thread_pool.hpp>
#include <engine/ev/thread_pool_config.hpp>
#include <userver/storages/redis/reply.hpp>
#include <userver/utest/utest.hpp>

#include "command.hpp"
#include "sentinel.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {
namespace {

enum class SubscriptionKind { kSubscribe, kPsubscribe, kSsubscribe };

struct RecoveryTestParams {
    bool cluster;
    SubscriptionKind kind;
    std::string name;
};

class SubscriptionRecovery : public ::testing::TestWithParam<RecoveryTestParams> {
public:
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}

protected:
    void SetUp() override {
        constexpr std::size_t kShardsCount = 1;
        if (GetParam().cluster) {
            storage_ = std::make_shared<ClusterSubscriptionStorage>(thread_pool_.NextThread(), kShardsCount);
        } else {
            storage_ = std::make_shared<SubscriptionStorage>(
                thread_pool_.NextThread(),
                kShardsCount,
                false,
                std::make_shared<const std::vector<std::string>>(std::vector<std::string>{"shard0"})
            );
        }
        storage_->SetSubscribeCallback([this](std::size_t, CommandPtr command) {
            subscribe_commands_.push_back(std::move(command));
        });
        storage_->SetShardedSubscribeCallback([this](const std::string&, CommandPtr command) {
            subscribe_commands_.push_back(std::move(command));
        });
        storage_->SetUnsubscribeCallback([this](std::size_t, CommandPtr command) {
            unsubscribe_commands_.push_back(std::move(command));
        });
        storage_->SetShardedUnsubscribeCallback([this](const std::string&, CommandPtr command) {
            unsubscribe_commands_.push_back(std::move(command));
        });

        auto on_message = [this](const std::string&, const std::string&) {
            ++received_messages_;
            return Sentinel::Outcome::kOk;
        };
        switch (GetParam().kind) {
            case SubscriptionKind::kSubscribe:
                token_.emplace(storage_->Subscribe(kChannel, on_message, {}));
                break;
            case SubscriptionKind::kPsubscribe:
                token_.emplace(storage_->Psubscribe(
                    kChannel,
                    [on_message](const std::string&, const std::string& channel, const std::string& message) {
                        return on_message(channel, message);
                    },
                    {}
                ));
                break;
            case SubscriptionKind::kSsubscribe:
                token_.emplace(storage_->Ssubscribe(kChannel, on_message, {}));
                break;
        }
        old_command_ = PopSubscribe(ServerId());
        ASSERT_TRUE(old_command_);
        CompleteSubscribe(old_command_, old_server_);
        EXPECT_EQ(Publish(), 1);
    }

    void TearDown() override {
        token_.reset();
        storage_->Stop();
        storage_.reset();
    }

    void Flush() const { static_cast<void>(storage_->GetStatistics()); }

    CommandPtr PopSubscribe(ServerId expected_server) {
        Flush();
        EXPECT_EQ(subscribe_commands_.size(), 1);
        if (subscribe_commands_.empty()) {
            return {};
        }
        auto command = std::move(subscribe_commands_.front());
        subscribe_commands_.erase(subscribe_commands_.begin());
        EXPECT_EQ(command->control.force_server_id, expected_server);
        return command;
    }

    void Rebalance() {
        storage_->DoRebalance(0, {{target_server_, 1}});
        Flush();
    }

    void RebalanceFromAvailableServer() {
        // Keep the old server available, but deterministically move the channel to the target.
        storage_->DoRebalance(0, {{old_server_, 0}, {target_server_, 1}});
        Flush();
    }

    void SendReply(const CommandPtr& command, ServerId server_id, ReplyData::Array data) {
        auto reply = std::make_shared<Reply>("pubsub", ReplyData(std::move(data)));
        reply->server_id = server_id;
        command->callback(command, std::move(reply));
        Flush();
    }

    void CompleteSubscribe(const CommandPtr& command, ServerId server_id) {
        active_subscriptions_[server_id] = command;
        const auto [command_name, channel] = command->args.GetCommandAndChannel();
        SendReply(command, server_id, {ReplyData(command_name), ReplyData(channel), ReplyData(1)});
    }

    void FailSubscribe(const CommandPtr& command, ServerId server_id) {
        auto reply = std::make_shared<
            Reply>("pubsub", ReplyData::CreateError("network unavailable"), ReplyStatus::kEndOfFileError);
        reply->server_id = server_id;
        command->callback(command, std::move(reply));
        Flush();
    }

    void CompleteOldUnsubscribe() {
        Flush();
        ASSERT_EQ(unsubscribe_commands_.size(), 1);
        const auto command = std::move(unsubscribe_commands_.front());
        unsubscribe_commands_.clear();
        EXPECT_EQ(command->control.force_server_id, old_server_);
        const auto [command_name, channel] = command->args.GetCommandAndChannel();
        const auto* expected_command =
            GetParam().kind == SubscriptionKind::kSubscribe ? "UNSUBSCRIBE"
            : GetParam().kind == SubscriptionKind::kPsubscribe
                ? "PUNSUBSCRIBE"
                : "SUNSUBSCRIBE";
        EXPECT_EQ(command_name, expected_command);
        EXPECT_EQ(channel, kChannel);
        active_subscriptions_.erase(old_server_);
        SendReply(old_command_, old_server_, {ReplyData(command_name), ReplyData(kChannel), ReplyData(0)});
    }

    std::size_t Publish() {
        const auto before = received_messages_;
        for (const auto& [server_id, command] : active_subscriptions_) {
            ReplyData::Array data;
            if (GetParam().kind == SubscriptionKind::kPsubscribe) {
                data = {ReplyData("pmessage"), ReplyData(kChannel), ReplyData(kChannel), ReplyData("payload")};
            } else {
                data = {
                    ReplyData(GetParam().kind == SubscriptionKind::kSsubscribe ? "smessage" : "message"),
                    ReplyData(kChannel),
                    ReplyData("payload"),
                };
            }
            SendReply(command, server_id, std::move(data));
        }
        return received_messages_ - before;
    }

    ServerId GetTargetServer() const { return target_server_; }
    ServerId GetReplacementServer() const { return replacement_server_; }

    bool HasPendingSubscribe() const { return !subscribe_commands_.empty(); }
    std::size_t GetPendingUnsubscribeCount() const { return unsubscribe_commands_.size(); }

    void DisconnectOldServer() {
        active_subscriptions_.erase(old_server_);
        FailSubscribe(old_command_, old_server_);
    }

private:
    static constexpr const char* kChannel = "recovery-channel";
    engine::ev::ThreadPool thread_pool_{engine::ev::ThreadPoolConfig{1, "redis_recovery_test"}};
    std::shared_ptr<SubscriptionStorageBase> storage_;
    std::optional<SubscriptionToken> token_;
    std::vector<CommandPtr> subscribe_commands_;
    std::vector<CommandPtr> unsubscribe_commands_;
    std::map<ServerId, CommandPtr> active_subscriptions_;
    std::size_t received_messages_{0};
    const ServerId old_server_{ServerId::Generate()};
    const ServerId target_server_{ServerId::Generate()};
    const ServerId replacement_server_{ServerId::Generate()};
    CommandPtr old_command_;
};

UTEST_P(SubscriptionRecovery, StopsDuplicateDeliveryAfterReplacement) {
    Rebalance();
    const auto replacement = PopSubscribe(GetTargetServer());
    ASSERT_TRUE(replacement);
    EXPECT_EQ(GetPendingUnsubscribeCount(), 0);
    EXPECT_EQ(Publish(), 1);

    CompleteSubscribe(replacement, GetTargetServer());
    EXPECT_EQ(GetPendingUnsubscribeCount(), 1);
    EXPECT_EQ(Publish(), 2);
    if (GetPendingUnsubscribeCount() != 0) {
        CompleteOldUnsubscribe();
    }
    EXPECT_EQ(Publish(), 1);
    EXPECT_FALSE(HasPendingSubscribe());
}

UTEST_P(SubscriptionRecovery, FailedTargetRetriesAndCleansOldSubscription) {
    Rebalance();
    const auto target = PopSubscribe(GetTargetServer());
    ASSERT_TRUE(target);
    FailSubscribe(target, GetTargetServer());
    const auto retry = PopSubscribe(ServerId());
    ASSERT_TRUE(retry);
    EXPECT_EQ(GetPendingUnsubscribeCount(), 0);
    EXPECT_EQ(Publish(), 1);

    CompleteSubscribe(retry, GetReplacementServer());
    ASSERT_EQ(GetPendingUnsubscribeCount(), 1);
    CompleteOldUnsubscribe();
    EXPECT_EQ(Publish(), 1);
    EXPECT_FALSE(HasPendingSubscribe());
}

UTEST_P(SubscriptionRecovery, SuccessfulRebalanceCleansOldSubscription) {
    RebalanceFromAvailableServer();
    const auto target = PopSubscribe(GetTargetServer());
    ASSERT_TRUE(target);
    EXPECT_EQ(GetPendingUnsubscribeCount(), 0);
    EXPECT_EQ(Publish(), 1);

    CompleteSubscribe(target, GetTargetServer());
    ASSERT_EQ(GetPendingUnsubscribeCount(), 1);
    EXPECT_EQ(Publish(), 2);
    CompleteOldUnsubscribe();
    EXPECT_EQ(GetPendingUnsubscribeCount(), 0);
    EXPECT_EQ(Publish(), 1);
    EXPECT_FALSE(HasPendingSubscribe());
}

UTEST_P(SubscriptionRecovery, OldDisconnectDoesNotDuplicatePendingSubscribe) {
    Rebalance();
    const auto replacement = PopSubscribe(GetTargetServer());
    ASSERT_TRUE(replacement);
    DisconnectOldServer();
    EXPECT_FALSE(HasPendingSubscribe());

    CompleteSubscribe(replacement, GetTargetServer());
    EXPECT_EQ(GetPendingUnsubscribeCount(), 0);
    EXPECT_EQ(Publish(), 1);
}

INSTANTIATE_UTEST_SUITE_P(
    /**/,
    SubscriptionRecovery,
    ::testing::Values(
        RecoveryTestParams{false, SubscriptionKind::kSubscribe, "SentinelSubscribe"},
        RecoveryTestParams{false, SubscriptionKind::kPsubscribe, "SentinelPsubscribe"},
        RecoveryTestParams{true, SubscriptionKind::kSubscribe, "ClusterSubscribe"},
        RecoveryTestParams{true, SubscriptionKind::kPsubscribe, "ClusterPsubscribe"},
        RecoveryTestParams{true, SubscriptionKind::kSsubscribe, "ClusterSsubscribe"}
    ),
    [](const auto& info) { return info.param.name; }
);

}  // namespace
}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
