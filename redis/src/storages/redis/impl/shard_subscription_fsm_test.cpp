#include "shard_subscription_fsm.hpp"

#include <string>
#include <utility>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl::shard_subscriber {

namespace {

ServerId MakeServerId(std::string description) {
    auto server_id = ServerId::Generate();
    server_id.SetDescription(std::move(description));
    return server_id;
}

void EstablishSubscription(Fsm& fsm, const ServerId& server_id) {
    const auto initial_actions = fsm.PopAllPendingActions();
    ASSERT_EQ(initial_actions.size(), 1);
    EXPECT_EQ(initial_actions.front().type, Action::Type::kSubscribe);
    EXPECT_TRUE(initial_actions.front().server_id.IsAny());

    fsm.OnEvent(Event{.type = Event::Type::kSubscribeReplyOk, .server_id = server_id});
    EXPECT_TRUE(fsm.PopAllPendingActions().empty());
    EXPECT_EQ(fsm.GetCurrentServerId(), server_id);
    EXPECT_TRUE(fsm.CanBeRebalanced());
}

}  // namespace

TEST(ShardSubscriptionFsm, FailedRebalanceKeepsAvailableCurrentServer) {
    const auto current_server = MakeServerId("current");
    const auto target_server = MakeServerId("target");
    Fsm fsm{0};
    EstablishSubscription(fsm, current_server);

    fsm.OnEvent(Event{
        .type = Event::Type::kRebalanceRequested,
        .server_id = target_server,
        .current_server_available = true,
    });
    auto actions = fsm.PopAllPendingActions();
    ASSERT_EQ(actions.size(), 1);
    EXPECT_EQ(actions.front().type, Action::Type::kSubscribe);
    EXPECT_EQ(actions.front().server_id, target_server);

    fsm.OnEvent(Event{.type = Event::Type::kSubscribeReplyError, .server_id = target_server});
    EXPECT_TRUE(fsm.PopAllPendingActions().empty());
    EXPECT_EQ(fsm.GetCurrentServerId(), current_server);
    EXPECT_TRUE(fsm.CanBeRebalanced());
}

TEST(ShardSubscriptionFsm, FailedRecoveryRetriesAnyServer) {
    const auto unavailable_server = MakeServerId("unavailable");
    const auto target_server = MakeServerId("target");
    const auto replacement_server = MakeServerId("replacement");
    Fsm fsm{0};
    EstablishSubscription(fsm, unavailable_server);

    fsm.OnEvent(Event{
        .type = Event::Type::kRebalanceRequested,
        .server_id = target_server,
        .current_server_available = false,
    });
    auto actions = fsm.PopAllPendingActions();
    ASSERT_EQ(actions.size(), 1);
    EXPECT_EQ(actions.front().type, Action::Type::kSubscribe);
    EXPECT_EQ(actions.front().server_id, target_server);
    EXPECT_TRUE(fsm.GetCurrentServerId().IsAny());
    EXPECT_FALSE(fsm.CanBeRebalanced());

    fsm.OnEvent(Event{.type = Event::Type::kSubscribeReplyError, .server_id = target_server});
    actions = fsm.PopAllPendingActions();
    ASSERT_EQ(actions.size(), 1);
    EXPECT_EQ(actions.front().type, Action::Type::kSubscribe);
    EXPECT_TRUE(actions.front().server_id.IsAny());

    fsm.OnEvent(Event{.type = Event::Type::kSubscribeReplyOk, .server_id = replacement_server});
    EXPECT_TRUE(fsm.PopAllPendingActions().empty());
    EXPECT_EQ(fsm.GetCurrentServerId(), replacement_server);
    EXPECT_TRUE(fsm.CanBeRebalanced());
}

}  // namespace storages::redis::impl::shard_subscriber

USERVER_NAMESPACE_END
