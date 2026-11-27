#include "subscription_storage.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <engine/ev/thread_control.hpp>
#include <engine/ev/thread_pool.hpp>
#include <engine/ev/thread_pool_config.hpp>
#include <userver/storages/redis/reply.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/rate.hpp>

#include "cluster_subscription_storage.hpp"
#include "cluster_topology.hpp"
#include "command.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {
namespace {

enum class SubscriptionKind { kChannel, kPattern, kSharded };

struct StorageSettings {
    size_t shards;
    bool cluster;
    SubscriptionKind kind;
};

class SubscriptionStorageTest : public ::testing::TestWithParam<StorageSettings> {
protected:
    void SetUp() override {
        auto names = std::make_shared<std::vector<std::string>>();
        for (size_t shard = 0; shard < GetParam().shards; ++shard) {
            names->push_back("shard" + std::to_string(shard));
            server_ids_.push_back(ServerId::Generate());
        }
        if (GetParam().cluster) {
            storage_ = std::make_unique<ClusterSubscriptionStorage>(thread_control_, GetParam().shards);
        } else {
            storage_ = std::make_unique<SubscriptionStorage>(thread_control_, GetParam().shards, std::move(names));
        }
        storage_->SetSubscribeCallback([this](size_t shard, CommandPtr cmd) {
            subscriptions_.push_back({shard, std::move(cmd)});
        });
        storage_->SetUnsubscribeCallback([this](size_t shard, CommandPtr cmd) {
            unsubscriptions_.push_back({shard, std::move(cmd)});
        });
        storage_->SetShardedSubscribeCallback([this](const std::string&, CommandPtr cmd) {
            subscriptions_.push_back({0, std::move(cmd)});
        });
        storage_->SetShardedUnsubscribeCallback([this](const std::string&, CommandPtr cmd) {
            unsubscriptions_.push_back({0, std::move(cmd)});
        });
    }

    SubscriptionToken Subscribe(Sentinel::UserPmessageCallback callback) {
        if (GetParam().kind == SubscriptionKind::kPattern) {
            return storage_->Psubscribe(kPattern, std::move(callback), {});
        }
        auto on_message = [callback = std::move(callback)](const std::string& channel, const std::string& message) {
            return callback({}, channel, message);
        };
        if (GetParam().kind == SubscriptionKind::kSharded) {
            return storage_->Ssubscribe(kChannel, std::move(on_message), {});
        }
        return storage_->Subscribe(kChannel, std::move(on_message), {});
    }

    size_t ExpectedSubscriptions() const { return GetParam().cluster ? 1 : GetParam().shards; }

    const std::string& SubscriptionName() const {
        return GetParam().kind == SubscriptionKind::kPattern ? kPattern : kChannel;
    }

    void AcknowledgeSubscriptions() {
        for (const auto& subscription : subscriptions_) {
            const auto& [command, channel] = subscription.command->args.GetCommandAndChannel();
            SendReply(subscription, {ReplyData(command), ReplyData(channel), ReplyData(1)});
        }
    }

    void AcknowledgeUnsubscriptions() {
        for (const auto& unsubscription : unsubscriptions_) {
            const auto& [command, channel] = unsubscription.command->args.GetCommandAndChannel();
            for (const auto& subscription : subscriptions_) {
                if (subscription.shard == unsubscription.shard) {
                    SendReply(subscription, {ReplyData(command), ReplyData(channel), ReplyData(0)});
                }
            }
        }
    }

    void SendMessages() {
        for (const auto& subscription : subscriptions_) {
            ReplyData::Array data;
            switch (GetParam().kind) {
                case SubscriptionKind::kChannel:
                    data = {ReplyData("message"), ReplyData(kChannel), ReplyData(kMessage)};
                    break;
                case SubscriptionKind::kPattern:
                    data = {ReplyData("pmessage"), ReplyData(kPattern), ReplyData(kChannel), ReplyData(kMessage)};
                    break;
                case SubscriptionKind::kSharded:
                    data = {ReplyData("smessage"), ReplyData(kChannel), ReplyData(kMessage)};
                    break;
            }
            SendReply(subscription, std::move(data));
        }
    }

    void CheckArguments(const std::string& pattern, const std::string& channel, const std::string& message) const {
        EXPECT_EQ(pattern, GetParam().kind == SubscriptionKind::kPattern ? kPattern : "");
        EXPECT_EQ(channel, kChannel);
        EXPECT_EQ(message, kMessage);
    }

    inline static const std::string kChannel = "channel";
    inline static const std::string kPattern = "chan*";
    inline static const std::string kMessage = "payload";

    struct CommandRecord {
        size_t shard;
        CommandPtr command;
    };

    const std::vector<CommandRecord>& Subscriptions() const { return subscriptions_; }
    const std::vector<CommandRecord>& Unsubscriptions() const { return unsubscriptions_; }
    const SubscriptionStorageBase& Storage() const { return *storage_; }
    const ServerId& ServerIdAt(size_t shard) const { return server_ids_.at(shard); }

private:
    engine::ev::ThreadPool thread_pool_{engine::ev::ThreadPoolConfig{1, "subscription_storage_test"}};
    engine::ev::ThreadControl thread_control_{thread_pool_.NextThread()};
    std::vector<ServerId> server_ids_;
    std::vector<CommandRecord> subscriptions_;
    std::vector<CommandRecord> unsubscriptions_;
    std::unique_ptr<SubscriptionStorageBase> storage_;

    void SendReply(const CommandRecord& subscription, ReplyData::Array data) {
        auto reply = std::make_shared<Reply>("", ReplyData(std::move(data)));
        reply->server_id = server_ids_.at(GetParam().cluster ? 0 : subscription.shard);
        thread_control_.RunInEvLoopSyncWithResult([&] { subscription.command->callback({}, std::move(reply)); });
    }
};

// Both local subscribers share one Redis subscription per Sentinel group, or one in Cluster mode.
// Check routing and delivery to both callbacks. Removing the first subscriber keeps the Redis
// subscription alive; removing the last sends UNSUBSCRIBE and clears the channel statistics.
UTEST_P(SubscriptionStorageTest, SubscribersShareAllRequiredConnections) {
    size_t first_messages = 0;
    size_t second_messages = 0;
    auto first = Subscribe([&](const auto& pattern, const auto& channel, const auto& message) {
        CheckArguments(pattern, channel, message);
        ++first_messages;
        return Sentinel::Outcome::kOk;
    });
    auto second = Subscribe([&](const auto& pattern, const auto& channel, const auto& message) {
        CheckArguments(pattern, channel, message);
        ++second_messages;
        return Sentinel::Outcome::kOk;
    });

    ASSERT_EQ(Subscriptions().size(), ExpectedSubscriptions());
    for (size_t shard = 0; shard < ExpectedSubscriptions(); ++shard) {
        EXPECT_EQ(
            Subscriptions()[shard].shard,
            GetParam().cluster && GetParam().kind != SubscriptionKind::kSharded ? ClusterTopology::kUnknownShard : shard
        );
    }
    AcknowledgeSubscriptions();
    SendMessages();
    EXPECT_EQ(first_messages, ExpectedSubscriptions());
    EXPECT_EQ(second_messages, ExpectedSubscriptions());

    first.Unsubscribe();
    EXPECT_TRUE(Unsubscriptions().empty());
    SendMessages();
    EXPECT_EQ(first_messages, ExpectedSubscriptions());
    EXPECT_EQ(second_messages, 2 * ExpectedSubscriptions());

    second.Unsubscribe();
    ASSERT_EQ(Unsubscriptions().size(), ExpectedSubscriptions());
    AcknowledgeUnsubscriptions();
    for (const auto& shard : Storage().GetStatistics().by_shard) {
        EXPECT_TRUE(shard.by_channel.empty());
    }
}

// A throwing callback must not prevent delivery to the other subscribers or message accounting.
// Count the received message and its bytes once per Redis subscription, and account for discarded
// deliveries. Overflow is simulated by kOverflowDiscarded; this test does not fill a real queue.
UTEST_P(SubscriptionStorageTest, AccountsMessagesAndOverflowDespiteCallbackException) {
    size_t delivered = 0;
    auto throwing = Subscribe([](const auto&, const auto&, const auto&) -> Sentinel::Outcome {
        throw std::runtime_error("subscriber failed");
    });
    auto overflowing = Subscribe([](const auto&, const auto&, const auto&) {
        return Sentinel::Outcome::kOverflowDiscarded;
    });
    auto successful = Subscribe([&](const auto& pattern, const auto& channel, const auto& message) {
        CheckArguments(pattern, channel, message);
        ++delivered;
        return Sentinel::Outcome::kOk;
    });
    AcknowledgeSubscriptions();
    SendMessages();
    EXPECT_EQ(delivered, ExpectedSubscriptions());

    const auto stats = Storage().GetStatistics();
    ASSERT_EQ(stats.by_shard.size(), ExpectedSubscriptions());
    for (size_t shard = 0; shard < stats.by_shard.size(); ++shard) {
        const auto& channel = stats.by_shard[shard].by_channel.at(SubscriptionName());
        EXPECT_EQ(channel.messages_count, utils::statistics::Rate{1});
        EXPECT_EQ(channel.messages_size, utils::statistics::Rate{kMessage.size()});
        EXPECT_EQ(channel.messages_discarded, utils::statistics::Rate{1});
        EXPECT_EQ(channel.messages_alien_count, utils::statistics::Rate{0});
        EXPECT_EQ(channel.server_id, ServerIdAt(shard));
        if (!GetParam().cluster) {
            EXPECT_EQ(stats.by_shard[shard].shard_name, "shard" + std::to_string(shard));
        }
    }
}

// The internal callback removes its own subscription and acknowledges the Redis unsubscriptions.
// Delivery must finish safely even though the channel entry disappears during the callback;
// subsequent replies must not invoke the removed callback or recreate its statistics.
UTEST_P(SubscriptionStorageTest, CallbackCanRemoveItsSubscription) {
    std::optional<SubscriptionToken> token;
    size_t delivered = 0;
    token.emplace(Subscribe([&](const auto& pattern, const auto& channel, const auto& message) {
        token->Unsubscribe();
        AcknowledgeUnsubscriptions();
        CheckArguments(pattern, channel, message);
        ++delivered;
        return Sentinel::Outcome::kOk;
    }));
    AcknowledgeSubscriptions();
    SendMessages();
    EXPECT_EQ(delivered, 1);
    for (const auto& shard : Storage().GetStatistics().by_shard) {
        EXPECT_TRUE(shard.by_channel.empty());
    }
}

// Run all three scenarios for Sentinel SUBSCRIBE/PSUBSCRIBE with one or two groups, and for
// Cluster SUBSCRIBE/PSUBSCRIBE/SSUBSCRIBE with three shards: seven configurations, 21 cases.
INSTANTIATE_UTEST_SUITE_P(
    Topologies,
    SubscriptionStorageTest,
    ::testing::Values(
        StorageSettings{1, false, SubscriptionKind::kChannel},
        StorageSettings{2, false, SubscriptionKind::kChannel},
        StorageSettings{1, false, SubscriptionKind::kPattern},
        StorageSettings{2, false, SubscriptionKind::kPattern},
        StorageSettings{3, true, SubscriptionKind::kChannel},
        StorageSettings{3, true, SubscriptionKind::kPattern},
        StorageSettings{3, true, SubscriptionKind::kSharded}
    )
);

}  // namespace
}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
