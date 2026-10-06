#include "shard_subscription_fsm.hpp"

#include <cstddef>
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

class ShardSubscriptionFsm : public ::testing::Test {
protected:
    void SetUp() override {
        ExpectSubscribe(ServerId());
        old_request_id_ = pending_request_id_;
        Reply(Event::Type::kSubscribeReplyOk, old_server_);
        EXPECT_TRUE(fsm_.PopAllPendingActions().empty());
        EXPECT_EQ(fsm_.GetCurrentServerId(), old_server_);
        EXPECT_TRUE(fsm_.CanBeRebalanced());
    }

    void StartRecovery() {
        fsm_.OnEvent(Event{
            .type = Event::Type::kRebalanceRequested,
            .server_id = target_server_,
            .current_server_available = false,
        });
        ExpectSubscribe(target_server_);
        EXPECT_FALSE(fsm_.CanBeRebalanced());
    }

    void StartRebalance() {
        fsm_.OnEvent(Event{.type = Event::Type::kRebalanceRequested, .server_id = target_server_});
        ExpectSubscribe(target_server_);
        EXPECT_FALSE(fsm_.CanBeRebalanced());
    }

    void ReconnectOldServer() {
        DisconnectOldServer();
        ExpectSubscribe(ServerId());
        Reply(Event::Type::kSubscribeReplyOk, old_server_);
        EXPECT_TRUE(fsm_.PopAllPendingActions().empty());
        EXPECT_TRUE(fsm_.CanBeRebalanced());
    }

    void ExpectSubscribe(ServerId server_id) {
        const auto actions = fsm_.PopAllPendingActions();
        ASSERT_EQ(actions.size(), 1);
        EXPECT_EQ(actions.front().type, Action::Type::kSubscribe);
        EXPECT_EQ(actions.front().server_id, server_id);
        pending_request_id_ = actions.front().subscribe_request_id;
    }

    void ExpectUnsubscribe(ServerId server_id) {
        const auto actions = fsm_.PopAllPendingActions();
        ASSERT_EQ(actions.size(), 1);
        EXPECT_EQ(actions.front().type, Action::Type::kUnsubscribe);
        EXPECT_EQ(actions.front().server_id, server_id);
        EXPECT_FALSE(fsm_.CanBeRebalanced());
    }

    void ExpectDeleted() {
        const auto actions = fsm_.PopAllPendingActions();
        ASSERT_EQ(actions.size(), 1);
        EXPECT_EQ(actions.front().type, Action::Type::kDeleteFsm);
    }

    void Reply(Event::Type type, ServerId server_id) { Reply(type, server_id, pending_request_id_); }

    void Reply(Event::Type type, ServerId server_id, std::size_t request_id) {
        fsm_.OnEvent(Event{.type = type, .server_id = server_id, .subscribe_request_id = request_id});
    }

    void DisconnectOldServer() {
        fsm_.OnEvent(Event{
            .type = Event::Type::kSubscribeReplyError,
            .server_id = old_server_,
            .subscribe_request_id = old_request_id_,
        });
    }

    void Unsubscribe() { fsm_.OnEvent(Event{.type = Event::Type::kUnsubscribeRequested, .server_id = {}}); }

    Fsm& GetFsm() { return fsm_; }

    ServerId GetOldServer() const { return old_server_; }
    ServerId GetTargetServer() const { return target_server_; }
    ServerId GetReplacementServer() const { return replacement_server_; }
    std::size_t GetPendingRequestId() const { return pending_request_id_; }
    std::size_t GetOldRequestId() const { return old_request_id_; }

private:
    Fsm fsm_{0};
    const ServerId old_server_{MakeServerId("old")};
    const ServerId target_server_{MakeServerId("target")};
    const ServerId replacement_server_{MakeServerId("replacement")};
    std::size_t pending_request_id_{0};
    std::size_t old_request_id_{0};
};

}  // namespace

TEST_F(ShardSubscriptionFsm, FailedRebalanceKeepsAvailableCurrentServer) {
    GetFsm().OnEvent(Event{
        .type = Event::Type::kRebalanceRequested,
        .server_id = GetTargetServer(),
        .current_server_available = true,
    });
    ExpectSubscribe(GetTargetServer());

    Reply(Event::Type::kSubscribeReplyError, GetTargetServer());
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    EXPECT_EQ(GetFsm().GetCurrentServerId(), GetOldServer());
    EXPECT_TRUE(GetFsm().CanBeRebalanced());

    DisconnectOldServer();
    ExpectSubscribe(ServerId());
}

TEST_F(ShardSubscriptionFsm, RecoveryUnsubscribesOldServerAfterReplacement) {
    StartRecovery();
    Reply(Event::Type::kSubscribeReplyOk, GetTargetServer());
    ExpectUnsubscribe(GetOldServer());
    EXPECT_EQ(GetFsm().GetCurrentServerId(), GetTargetServer());

    DisconnectOldServer();
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    EXPECT_TRUE(GetFsm().CanBeRebalanced());
}

TEST_F(ShardSubscriptionFsm, SuccessfulRebalanceWaitsForOldUnsubscribe) {
    StartRebalance();
    EXPECT_EQ(GetFsm().GetCurrentServerId(), GetOldServer());
    Reply(Event::Type::kSubscribeReplyOk, GetTargetServer());
    ExpectUnsubscribe(GetOldServer());
    EXPECT_EQ(GetFsm().GetCurrentServerId(), GetTargetServer());

    Reply(Event::Type::kSubscribeReplyError, GetOldServer(), GetOldRequestId());
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    EXPECT_TRUE(GetFsm().CanBeRebalanced());
    EXPECT_EQ(GetFsm().GetCurrentServerId(), GetTargetServer());

    Reply(Event::Type::kSubscribeReplyError, GetTargetServer());
    ExpectSubscribe(ServerId());
}

TEST_F(ShardSubscriptionFsm, OldSuccessDuringRecoveryDoesNotCompleteReplacement) {
    StartRecovery();
    Reply(Event::Type::kSubscribeReplyOk, GetOldServer(), GetOldRequestId());
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    EXPECT_TRUE(GetFsm().GetCurrentServerId().IsAny());
    EXPECT_FALSE(GetFsm().CanBeRebalanced());

    Reply(Event::Type::kSubscribeReplyOk, GetTargetServer());
    ExpectUnsubscribe(GetOldServer());
    DisconnectOldServer();
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    EXPECT_TRUE(GetFsm().CanBeRebalanced());
}

TEST_F(ShardSubscriptionFsm, SupersededSuccessDuringRetryIsUnsubscribed) {
    StartRecovery();
    const auto failed_request_id = GetPendingRequestId();
    Reply(Event::Type::kSubscribeReplyError, GetTargetServer());
    ExpectSubscribe(ServerId());

    Reply(Event::Type::kSubscribeReplyOk, GetTargetServer(), failed_request_id);
    ExpectUnsubscribe(GetTargetServer());
    EXPECT_TRUE(GetFsm().GetCurrentServerId().IsAny());
    Reply(Event::Type::kSubscribeReplyError, GetTargetServer(), failed_request_id);
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    EXPECT_FALSE(GetFsm().CanBeRebalanced());

    Reply(Event::Type::kSubscribeReplyOk, GetReplacementServer());
    ExpectUnsubscribe(GetOldServer());
    DisconnectOldServer();
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    EXPECT_EQ(GetFsm().GetCurrentServerId(), GetReplacementServer());
    EXPECT_TRUE(GetFsm().CanBeRebalanced());
}

TEST_F(ShardSubscriptionFsm, RebalanceWaitSubscribeIgnoresPreviousCurrentDisconnect) {
    ReconnectOldServer();
    const auto current_request_id = GetPendingRequestId();
    StartRebalance();

    DisconnectOldServer();
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    EXPECT_EQ(GetFsm().GetCurrentServerId(), GetOldServer());
    EXPECT_FALSE(GetFsm().CanBeRebalanced());

    Reply(Event::Type::kSubscribeReplyError, GetTargetServer());
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    EXPECT_TRUE(GetFsm().CanBeRebalanced());
    Reply(Event::Type::kSubscribeReplyError, GetOldServer(), current_request_id);
    ExpectSubscribe(ServerId());
}

TEST_F(ShardSubscriptionFsm, RebalanceWaitSubscribeIgnoresPreviousTargetFailure) {
    StartRebalance();
    const auto failed_request_id = GetPendingRequestId();
    Reply(Event::Type::kSubscribeReplyError, GetTargetServer());
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    StartRebalance();

    Reply(Event::Type::kSubscribeReplyError, GetTargetServer(), failed_request_id);
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    EXPECT_EQ(GetFsm().GetCurrentServerId(), GetOldServer());
    EXPECT_FALSE(GetFsm().CanBeRebalanced());

    Reply(Event::Type::kSubscribeReplyOk, GetTargetServer());
    ExpectUnsubscribe(GetOldServer());
    DisconnectOldServer();
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    EXPECT_TRUE(GetFsm().CanBeRebalanced());
}

TEST_F(ShardSubscriptionFsm, RebalanceWaitSubscribeDoesNotCompleteOnPreviousTargetSuccess) {
    StartRebalance();
    const auto failed_request_id = GetPendingRequestId();
    Reply(Event::Type::kSubscribeReplyError, GetTargetServer());
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    StartRebalance();

    Reply(Event::Type::kSubscribeReplyOk, GetTargetServer(), failed_request_id);
    ExpectUnsubscribe(GetTargetServer());
    EXPECT_EQ(GetFsm().GetCurrentServerId(), GetOldServer());
    Reply(Event::Type::kSubscribeReplyError, GetTargetServer(), failed_request_id);
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    EXPECT_FALSE(GetFsm().CanBeRebalanced());

    Reply(Event::Type::kSubscribeReplyOk, GetTargetServer());
    ExpectUnsubscribe(GetOldServer());
    DisconnectOldServer();
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    EXPECT_TRUE(GetFsm().CanBeRebalanced());
}

TEST_F(ShardSubscriptionFsm, RebalanceWaitUnsubscribeIgnoresPreviousCurrentFailure) {
    StartRebalance();
    const auto failed_request_id = GetPendingRequestId();
    Reply(Event::Type::kSubscribeReplyError, GetTargetServer());
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    StartRebalance();
    Reply(Event::Type::kSubscribeReplyOk, GetTargetServer());
    ExpectUnsubscribe(GetOldServer());

    Reply(Event::Type::kSubscribeReplyError, GetTargetServer(), failed_request_id);
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    EXPECT_EQ(GetFsm().GetCurrentServerId(), GetTargetServer());
    EXPECT_FALSE(GetFsm().CanBeRebalanced());

    DisconnectOldServer();
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    EXPECT_TRUE(GetFsm().CanBeRebalanced());
    Reply(Event::Type::kSubscribeReplyError, GetTargetServer());
    ExpectSubscribe(ServerId());
}

TEST_F(ShardSubscriptionFsm, RebalanceWaitUnsubscribeIgnoresPreviousOldFailure) {
    ReconnectOldServer();
    const auto current_request_id = GetPendingRequestId();
    StartRebalance();
    Reply(Event::Type::kSubscribeReplyOk, GetTargetServer());
    ExpectUnsubscribe(GetOldServer());

    DisconnectOldServer();
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    EXPECT_EQ(GetFsm().GetCurrentServerId(), GetTargetServer());
    EXPECT_FALSE(GetFsm().CanBeRebalanced());

    Reply(Event::Type::kSubscribeReplyError, GetOldServer(), current_request_id);
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    EXPECT_TRUE(GetFsm().CanBeRebalanced());
    Reply(Event::Type::kSubscribeReplyError, GetTargetServer());
    ExpectSubscribe(ServerId());
}

TEST_F(ShardSubscriptionFsm, FailedRecoveryRetriesAnyServer) {
    StartRecovery();
    Reply(Event::Type::kSubscribeReplyError, GetTargetServer());
    ExpectSubscribe(ServerId());
    Reply(Event::Type::kSubscribeReplyError, GetReplacementServer());
    ExpectSubscribe(ServerId());

    Reply(Event::Type::kSubscribeReplyOk, GetReplacementServer());
    ExpectUnsubscribe(GetOldServer());
    EXPECT_EQ(GetFsm().GetCurrentServerId(), GetReplacementServer());

    DisconnectOldServer();
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    EXPECT_TRUE(GetFsm().CanBeRebalanced());
}

TEST_F(ShardSubscriptionFsm, OldDisconnectDoesNotStartAnotherRecovery) {
    StartRecovery();
    DisconnectOldServer();
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());

    Reply(Event::Type::kSubscribeReplyOk, GetTargetServer());
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    EXPECT_EQ(GetFsm().GetCurrentServerId(), GetTargetServer());
    EXPECT_TRUE(GetFsm().CanBeRebalanced());
}

TEST_F(ShardSubscriptionFsm, OldDisconnectWhileRetryingDoesNotStartAnotherAttempt) {
    StartRecovery();
    Reply(Event::Type::kSubscribeReplyError, GetTargetServer());
    ExpectSubscribe(ServerId());
    DisconnectOldServer();
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());

    Reply(Event::Type::kSubscribeReplyError, GetOldServer());
    ExpectSubscribe(ServerId());
    Reply(Event::Type::kSubscribeReplyOk, GetReplacementServer());
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    EXPECT_TRUE(GetFsm().CanBeRebalanced());
}

TEST_F(ShardSubscriptionFsm, RetryFailureOnOldServerDoesNotHideFailedAttempt) {
    StartRecovery();
    Reply(Event::Type::kSubscribeReplyError, GetTargetServer());
    ExpectSubscribe(ServerId());
    Reply(Event::Type::kSubscribeReplyError, GetOldServer());
    ExpectSubscribe(ServerId());
    DisconnectOldServer();
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());

    Reply(Event::Type::kSubscribeReplyOk, GetReplacementServer());
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    EXPECT_TRUE(GetFsm().CanBeRebalanced());
}

TEST_F(ShardSubscriptionFsm, RetryReturningToOldServerDoesNotUnsubscribeIt) {
    StartRecovery();
    Reply(Event::Type::kSubscribeReplyError, GetTargetServer());
    ExpectSubscribe(ServerId());
    Reply(Event::Type::kSubscribeReplyOk, GetOldServer());
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    EXPECT_EQ(GetFsm().GetCurrentServerId(), GetOldServer());
    EXPECT_TRUE(GetFsm().CanBeRebalanced());

    DisconnectOldServer();
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    Reply(Event::Type::kSubscribeReplyError, GetOldServer());
    ExpectSubscribe(ServerId());
}

TEST_F(ShardSubscriptionFsm, UnsubscribeDuringRecoveryCleansBothServers) {
    StartRecovery();
    Unsubscribe();
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());

    Reply(Event::Type::kSubscribeReplyOk, GetTargetServer());
    ExpectUnsubscribe(GetOldServer());
    DisconnectOldServer();
    ExpectUnsubscribe(GetTargetServer());
    Reply(Event::Type::kSubscribeReplyError, GetTargetServer());
    ExpectDeleted();
}

TEST_F(ShardSubscriptionFsm, UnsubscribeAfterReturningToOldServerIgnoresOldReply) {
    StartRecovery();
    Reply(Event::Type::kSubscribeReplyError, GetTargetServer());
    ExpectSubscribe(ServerId());
    Reply(Event::Type::kSubscribeReplyOk, GetOldServer());
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());

    Unsubscribe();
    ExpectUnsubscribe(GetOldServer());
    DisconnectOldServer();
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    Reply(Event::Type::kSubscribeReplyError, GetOldServer());
    ExpectDeleted();
}

TEST_F(ShardSubscriptionFsm, UnsubscribeDuringFailedRecoveryCleansOldServer) {
    StartRecovery();
    Unsubscribe();
    Reply(Event::Type::kSubscribeReplyError, GetTargetServer());
    ExpectUnsubscribe(GetOldServer());
    DisconnectOldServer();
    ExpectDeleted();
}

TEST_F(ShardSubscriptionFsm, UnsubscribeWaitsForPendingRecoveryAfterOldDisconnect) {
    StartRecovery();
    Unsubscribe();
    DisconnectOldServer();
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    Reply(Event::Type::kSubscribeReplyError, GetTargetServer());
    ExpectDeleted();
}

TEST_F(ShardSubscriptionFsm, ReplacementDisconnectWaitsForOldUnsubscribeBeforeRetrying) {
    StartRecovery();
    Reply(Event::Type::kSubscribeReplyOk, GetTargetServer());
    ExpectUnsubscribe(GetOldServer());
    Reply(Event::Type::kSubscribeReplyError, GetTargetServer());
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    DisconnectOldServer();
    ExpectSubscribe(ServerId());
}

TEST_F(ShardSubscriptionFsm, RebalanceDisconnectRecoversThroughPendingAttempt) {
    GetFsm().OnEvent(Event{.type = Event::Type::kRebalanceRequested, .server_id = GetTargetServer()});
    ExpectSubscribe(GetTargetServer());
    DisconnectOldServer();
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    Reply(Event::Type::kSubscribeReplyError, GetTargetServer());
    ExpectSubscribe(ServerId());
    Reply(Event::Type::kSubscribeReplyOk, GetReplacementServer());
    EXPECT_TRUE(GetFsm().PopAllPendingActions().empty());
    EXPECT_TRUE(GetFsm().CanBeRebalanced());
}

}  // namespace storages::redis::impl::shard_subscriber

USERVER_NAMESPACE_END
