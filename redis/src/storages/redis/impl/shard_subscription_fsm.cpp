#include "shard_subscription_fsm.hpp"

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl::shard_subscriber {

std::string Event::TypeToDebugString(Type type) {
    switch (type) {
        case Type::kSubscribeRequested:
            return "kSubscribeRequested";
        case Type::kSubscribeReplyOk:
            return "kSubscribeReplyOk";
        case Type::kSubscribeReplyError:
            return "kSubscribeReplyError";
        case Type::kRebalanceRequested:
            return "kRebalanceRequested";
        case Type::kUnsubscribeRequested:
            return "kUnsubscribeRequested";
    }

    return "(unknown)";
}

std::string Event::ToDebugString() const {
    auto result = "Event(" + TypeToDebugString(type) + ", server_id=" + std::to_string(server_id.GetId());
    if (type == Type::kRebalanceRequested) {
        result += ", current_server_available=" + std::to_string(current_server_available);
    } else if (type == Type::kSubscribeReplyOk || type == Type::kSubscribeReplyError) {
        // A reply may belong to an older attempt, including one on the same server.
        result += ", subscribe_request_id=" + std::to_string(subscribe_request_id);
    }
    return result + ')';
}

std::string Action::TypeToDebugString(Type type) {
    switch (type) {
        case Type::kSubscribe:
            return "kSubscribe";
        case Type::kUnsubscribe:
            return "kUnsubscribe";
        case Type::kDeleteFsm:
            return "kDeleteFsm";
    }

    return "(unknown)";
}

std::string Action::ToDebugString() const {
    return "Action(" + TypeToDebugString(type) + ", server_id=" + std::to_string(server_id.GetId()) +
           ", subscribe_request_id=" + std::to_string(subscribe_request_id) + ")";
}

Fsm::Fsm(size_t shard, ServerId server_id)
    : shard_(shard),
      current_server_id_(server_id)
{
    // try to subscribe to anybody by default or to fixed server_id if specified
    EmitAction(Action(Action::Type::kSubscribe, server_id));
}

size_t Fsm::GetShard() const { return shard_; }

void Fsm::OnEvent(const Event& event) {
    LOG_DEBUG()
        << "OnEvent fsm=" << static_cast<void*>(this) << " state=" << StateToDebugString(state_)
        << " (current_server_id=" << current_server_id_.GetId() << ")"
        << " (rebalancing_server_id=" << rebalancing_server_id_.GetId() << ")"
        << " event=" << event.ToDebugString();

    switch (state_) {
        case State::kSubscribing:
            HandleSubscribing(event);
            return;

        case State::kSubscribed:
            HandleSubscribed(event);
            return;

        case State::kUnsubscribing:
            HandleUnsubscribing(event);
            return;

        case State::kRebalancingWaitSubscribe:
            HandleRebalancingWaitSubscribe(event);
            return;

        case State::kRebalancingWaitUnsubscribe:
            HandleRebalancingWaitUnsubscribe(event);
            return;

        case State::kUnsubscribed:
            HandleUnsubscribed(event);
            return;
    }
}

std::vector<Action> Fsm::PopAllPendingActions() {
    std::vector<Action> actions;
    swap(actions, pending_actions_);
    return actions;
}

ServerId Fsm::GetCurrentServerId() const { return current_server_id_; }

bool Fsm::CanBeRebalanced() const { return state_ == State::kSubscribed && need_subscription_; }

std::chrono::steady_clock::time_point Fsm::GetCurrentServerTimePoint() const { return current_server_subscription_tp_; }

void Fsm::SetNeedSubscription(bool need_subscription) {
    if (need_subscription_ == need_subscription) {
        return;
    }
    need_subscription_ = need_subscription;

    LOG_INFO()
        << "Fsm switch need_subscription: fsm=" << static_cast<void*>(this)
        << ", need_subscription=" << need_subscription_ << ", state=" << StateToDebugString(state_);
}

void Fsm::HandleSubscribing(const Event& event) {
    if (!current_server_id_.IsAny()) {
        LOG_ERROR() << "current_server_id_ must be 'any' in this place. Buggy fsm?";
        return;
    }
    // During recovery, rebalancing_server_id_ may still hold the old subscription.
    // Its replies must not complete or retry the pending replacement attempt.
    if (HandlePreviousSubscribeReply(event)) {
        return;
    }

    switch (event.type) {
        case Event::Type::kSubscribeRequested:
            SetNeedSubscription(true);
            break;

        case Event::Type::kSubscribeReplyOk:
            current_server_subscription_tp_ = std::chrono::steady_clock::now();
            current_server_id_ = event.server_id;
            current_subscribe_request_id_ = event.subscribe_request_id;
            // The replacement is confirmed, but the old subscription may still be
            // delivering messages on another server. Wait for its cleanup before
            // allowing another rebalance, even if the caller has unsubscribed meanwhile.
            if (!rebalancing_server_id_.IsAny() && rebalancing_server_id_ != current_server_id_) {
                EmitAction(Action(Action::Type::kUnsubscribe, rebalancing_server_id_));
                ChangeState(State::kRebalancingWaitUnsubscribe);
                break;
            }
            // The old subscription is already gone, or the retry selected the same
            // server. In the latter case, UNSUBSCRIBE would remove the new subscription too.
            rebalancing_server_id_ = ServerId();
            ChangeState(State::kSubscribed);
            LOG_DEBUG() << "Successfully subscribed to server_id=" << event.server_id.GetId();

            if (!need_subscription_) {
                EmitAction(Action(Action::Type::kUnsubscribe, current_server_id_));
                ChangeState(State::kUnsubscribing);
            }
            break;

        case Event::Type::kSubscribeReplyError:
            if (need_subscription_) {
                // We're stubborn, try again.
                // Redis driver should handle timeout, so relax is not needed.
                ChangeState(State::kSubscribing);
                LOG_WARNING()
                    << "Subscription to server_id=" << event.server_id.GetId() << " failed. try any server_id";
                EmitAction(Action(Action::Type::kSubscribe, ServerId()));
            } else if (!rebalancing_server_id_.IsAny()) {
                // The caller unsubscribed while recovery was pending, and the
                // replacement failed. The retained old subscription still needs cleanup.
                current_server_id_ = rebalancing_server_id_;
                current_subscribe_request_id_ = rebalancing_subscribe_request_id_;
                rebalancing_server_id_ = ServerId();
                EmitAction(Action(Action::Type::kUnsubscribe, current_server_id_));
                ChangeState(State::kUnsubscribing);
            } else {
                ChangeState(State::kUnsubscribed);
                EmitAction(Action::Type::kDeleteFsm);
            }
            break;

        case Event::Type::kRebalanceRequested:
            LOG_WARNING()
                << "Ignore rebalance requests while "
                   "current_server_id_.IsAny()=true";
            break;

        case Event::Type::kUnsubscribeRequested:
            SetNeedSubscription(false);
            break;
    }
}

bool Fsm::HandlePreviousSubscribeReply(const Event& event) {
    // Caller requests and replies to the latest attempt are handled by HandleSubscribing.
    // Matching the request ID also distinguishes a retry on the old server from its old subscription.
    if ((event.type != Event::Type::kSubscribeReplyOk && event.type != Event::Type::kSubscribeReplyError) ||
        event.subscribe_request_id == last_subscribe_request_id_)
    {
        return false;
    }

    // The retained old subscription can disconnect while its replacement is still
    // pending. Forget only the old subscription; the replacement needs no extra SUBSCRIBE.
    if (!rebalancing_server_id_.IsAny() && event.subscribe_request_id == rebalancing_subscribe_request_id_) {
        UASSERT(event.server_id == rebalancing_server_id_);
        // A duplicate success leaves the old subscription tracked for later cleanup.
        if (event.type == Event::Type::kSubscribeReplyError) {
            rebalancing_server_id_ = ServerId();
        }
    } else if (event.type == Event::Type::kSubscribeReplyOk) {
        // A superseded attempt succeeded unexpectedly and must be unsubscribed.
        HandleOkReplyFromOtherServerId(event.server_id);
    } else {
        // A superseded attempt ended; keep waiting for the current attempt's reply.
        HandleErrorReplyFromOtherServerId(event.server_id);
    }
    return true;
}

void Fsm::HandleSubscribed(const Event& event) {
    if (current_server_id_.IsAny()) {
        LOG_ERROR() << "current_server_id_ must not be 'any' in this place. Buggy fsm?";
        return;
    }
    if (!rebalancing_server_id_.IsAny()) {
        LOG_ERROR() << "rebalancing_server_id_ must be 'any' in this place. Buggy fsm?";
        return;
    }
    if (!need_subscription_) {
        LOG_ERROR() << "need_subscription_ must be true in this place. Buggy fsm?";
        return;
    }

    switch (event.type) {
        case Event::Type::kSubscribeRequested:
            LOG_WARNING() << "got kSubscribeRequested event when already subscribed";
            break;

        case Event::Type::kSubscribeReplyOk:
            if (event.server_id != current_server_id_) {
                HandleOkReplyFromOtherServerId(event.server_id);
            } else {
                LOG_WARNING() << "Duplicate OK subscribe reply from server_id=" << event.server_id.GetId();
                current_server_subscription_tp_ = std::chrono::steady_clock::now();
            }
            break;

        case Event::Type::kSubscribeReplyError:
            // Only the active subscription's disconnect starts recovery. After a retry
            // returns to the same server, a delayed disconnect from its old request is stale.
            if (event.server_id == current_server_id_ && event.subscribe_request_id == current_subscribe_request_id_) {
                // reset current instance, now we want to connect to any host
                current_server_id_ = ServerId();
                ChangeState(State::kSubscribing);
                EmitAction(Action(Action::Type::kSubscribe, ServerId()));
            } else {
                HandleErrorReplyFromOtherServerId(event.server_id);
            }
            break;

        case Event::Type::kRebalanceRequested:
            if (event.server_id != current_server_id_) {
                // A normal rebalance can keep the current subscription if the target fails.
                if (event.current_server_available) {
                    rebalancing_server_id_ = event.server_id;
                    ChangeState(State::kRebalancingWaitSubscribe);
                    EmitAction(Action(Action::Type::kSubscribe, rebalancing_server_id_));
                    rebalancing_subscribe_request_id_ = last_subscribe_request_id_;
                } else {
                    // Disappearing from the available-server weights does not close the
                    // old connection. Retain it for cleanup while retrying the replacement;
                    // falling back to this subscription after a failed attempt is not enough.
                    LOG_INFO()
                        << "Recover subscription from unavailable server_id=" << current_server_id_.GetId()
                        << " via server_id=" << event.server_id.GetId();
                    rebalancing_server_id_ = current_server_id_;
                    rebalancing_subscribe_request_id_ = current_subscribe_request_id_;
                    current_server_id_ = ServerId();
                    ChangeState(State::kSubscribing);
                    EmitAction(Action(Action::Type::kSubscribe, event.server_id));
                }
            } else {
                LOG_WARNING() << "Requested rebalance to the same server_id=" << event.server_id.GetId();
            }
            break;

        case Event::Type::kUnsubscribeRequested:
            SetNeedSubscription(false);
            EmitAction(Action(Action::Type::kUnsubscribe, current_server_id_));
            ChangeState(State::kUnsubscribing);
            break;
    }
}

void Fsm::HandleUnsubscribing(const Event& event) {
    if (current_server_id_.IsAny()) {
        LOG_ERROR() << "current_server_id_ must not be 'any' in this place. Buggy fsm?";
        return;
    }
    if (!rebalancing_server_id_.IsAny()) {
        LOG_ERROR() << "rebalancing_server_id_ must be 'any' in this place. Buggy fsm?";
        return;
    }

    switch (event.type) {
        case Event::Type::kSubscribeRequested:
            SetNeedSubscription(true);
            break;

        case Event::Type::kSubscribeReplyOk:
            if (event.server_id != current_server_id_) {
                HandleOkReplyFromOtherServerId(event.server_id);
            } else {
                LOG_WARNING() << "Duplicate OK subscribe reply from server_id=" << event.server_id.GetId();
                current_server_subscription_tp_ = std::chrono::steady_clock::now();
            }
            break;

        case Event::Type::kSubscribeReplyError:
            // Both an UNSUBSCRIBE acknowledgement and a disconnect arrive through the
            // original SUBSCRIBE callback. A reply from an older use of the same server
            // must not finish this wait or trigger a new subscription.
            if (event.server_id == current_server_id_ && event.subscribe_request_id == current_subscribe_request_id_) {
                current_server_id_ = ServerId();
                if (need_subscription_) {
                    // Subscription is needed again, or the replacement disconnected
                    // during cleanup. The old unsubscribe is now complete, so retrying is safe.
                    ChangeState(State::kSubscribing);
                    EmitAction(Action(Action::Type::kSubscribe, ServerId()));
                } else {
                    ChangeState(State::kUnsubscribed);
                    EmitAction(Action::Type::kDeleteFsm);
                }
            } else {
                HandleErrorReplyFromOtherServerId(event.server_id);
            }
            break;

        case Event::Type::kRebalanceRequested:
            LOG_WARNING() << "Ignore rebalance requests while unsubscribing";
            break;

        case Event::Type::kUnsubscribeRequested:
            SetNeedSubscription(false);
            break;
    }
}

void Fsm::HandleRebalancingWaitSubscribe(const Event& event) {
    if (current_server_id_.IsAny()) {
        LOG_ERROR() << "current_server_id_ must not be 'any' in this place. Buggy fsm?";
        return;
    }
    if (rebalancing_server_id_.IsAny()) {
        LOG_ERROR() << "rebalancing_server_id_ must not be 'any' in this place. Buggy fsm?";
        return;
    }

    switch (event.type) {
        case Event::Type::kSubscribeRequested:
            SetNeedSubscription(true);
            break;

        case Event::Type::kSubscribeReplyOk:
            if (event.server_id == current_server_id_) {
                LOG_WARNING()
                    << "Got successful Subscribe reply from server_id=" << event.server_id.GetId()
                    << " while already subscribed to it";
            } else if (event.server_id == rebalancing_server_id_ &&
                       event.subscribe_request_id == rebalancing_subscribe_request_id_)
            {
                // The pending target subscription is confirmed. It becomes current;
                // the rebalance slot now holds the old subscription awaiting UNSUBSCRIBE.
                current_server_subscription_tp_ = std::chrono::steady_clock::now();
                LOG_DEBUG()
                    << "Successfully subscribed to server_id=" << event.server_id.GetId() << " after rebalancing";
                std::swap(current_server_id_, rebalancing_server_id_);
                std::swap(current_subscribe_request_id_, rebalancing_subscribe_request_id_);
                EmitAction(Action(Action::Type::kUnsubscribe, rebalancing_server_id_));
                ChangeState(State::kRebalancingWaitUnsubscribe);
                // we can't make any subscribe request until we have got some
                // 'unsubscribe' confirmation from rebalancing_server_id_
            } else {
                HandleOkReplyFromOtherServerId(event.server_id);
            }
            break;

        case Event::Type::kSubscribeReplyError:
            // The current subscription disconnected before the target replied.
            // Continue with the already pending target attempt instead of issuing another one.
            if (event.server_id == current_server_id_ && event.subscribe_request_id == current_subscribe_request_id_) {
                current_server_id_ = ServerId();
                rebalancing_server_id_ = ServerId();
                ChangeState(State::kSubscribing);
            } else if (event.server_id == rebalancing_server_id_ &&
                       event.subscribe_request_id == rebalancing_subscribe_request_id_)
            {
                // The pending target attempt failed, but the current subscription is
                // still valid. Keep it unless the caller requested unsubscription meanwhile.
                LOG_WARNING() << "Rebalance subscription to server_id=" << rebalancing_server_id_.GetId() << " failed";
                rebalancing_server_id_ = ServerId();
                ChangeState(State::kSubscribed);
                if (need_subscription_) {
                    // stay connected to current_server_id_
                } else {
                    EmitAction(Action(Action::Type::kUnsubscribe, current_server_id_));
                    ChangeState(State::kUnsubscribing);
                }
            } else {
                HandleErrorReplyFromOtherServerId(event.server_id);
            }
            break;

        case Event::Type::kRebalanceRequested:
            LOG_INFO() << "Ignore rebalance request in " << StateToDebugString(state_) << " state";
            break;

        case Event::Type::kUnsubscribeRequested:
            SetNeedSubscription(false);
            break;
    }
}

void Fsm::HandleRebalancingWaitUnsubscribe(const Event& event) {
    if (current_server_id_.IsAny()) {
        LOG_ERROR() << "current_server_id_ must not be 'any' in this place. Buggy fsm?";
        return;
    }
    if (rebalancing_server_id_.IsAny()) {
        LOG_ERROR() << "rebalancing_server_id_ must not be 'any' in this place. Buggy fsm?";
        return;
    }

    switch (event.type) {
        case Event::Type::kSubscribeRequested:
            SetNeedSubscription(true);
            break;

        case Event::Type::kSubscribeReplyOk:
            if (event.server_id == current_server_id_ || event.server_id == rebalancing_server_id_) {
                LOG_WARNING()
                    << "Got successful Subscribe reply from server_id=" << event.server_id.GetId()
                    << " while already subscribed to it";
            } else {
                HandleOkReplyFromOtherServerId(event.server_id);
            }
            break;

        case Event::Type::kSubscribeReplyError:
            // The replacement disconnected before the old subscription's cleanup
            // completed. Keep waiting for the old UNSUBSCRIBE before retrying: a new
            // SUBSCRIBE on that same connection could receive the old unsubscribe reply.
            if (event.server_id == current_server_id_ && event.subscribe_request_id == current_subscribe_request_id_) {
                current_server_id_ = rebalancing_server_id_;
                current_subscribe_request_id_ = rebalancing_subscribe_request_id_;
                rebalancing_server_id_ = ServerId();
                ChangeState(State::kUnsubscribing);
            } else if (event.server_id == rebalancing_server_id_ &&
                       event.subscribe_request_id == rebalancing_subscribe_request_id_)
            {
                // The old subscription acknowledged UNSUBSCRIBE or disconnected.
                // The replacement remains active; unsubscribe it too if the caller cancelled.
                rebalancing_server_id_ = ServerId();
                ChangeState(State::kSubscribed);
                if (!need_subscription_) {
                    EmitAction(Action(Action::Type::kUnsubscribe, current_server_id_));
                    ChangeState(State::kUnsubscribing);
                }
            } else {
                HandleErrorReplyFromOtherServerId(event.server_id);
            }
            break;

        case Event::Type::kRebalanceRequested:
            LOG_INFO() << "Ignore rebalance request in " << StateToDebugString(state_) << " state";
            break;

        case Event::Type::kUnsubscribeRequested:
            SetNeedSubscription(false);
            break;
    }
}

void Fsm::HandleUnsubscribed(const Event& event) {
    if (!current_server_id_.IsAny()) {
        LOG_ERROR() << "current_server_id_ must be 'any' in this place. Buggy fsm?";
        return;
    }
    if (!rebalancing_server_id_.IsAny()) {
        LOG_ERROR() << "rebalancing_server_id_ must be 'any' in this place. Buggy fsm?";
        return;
    }
    if (need_subscription_) {
        LOG_ERROR() << "need_subscription_ must be false in this place. Buggy fsm?";
        return;
    }

    switch (event.type) {
        case Event::Type::kSubscribeRequested:
            SetNeedSubscription(true);
            EmitAction(Action(Action::Type::kSubscribe, ServerId()));
            ChangeState(State::kSubscribing);
            break;

        case Event::Type::kSubscribeReplyOk:
            HandleOkReplyFromOtherServerId(event.server_id);
            break;

        case Event::Type::kSubscribeReplyError:
            HandleErrorReplyFromOtherServerId(event.server_id);
            break;

        case Event::Type::kRebalanceRequested:
            LOG_INFO() << "Ignore rebalance request in " << StateToDebugString(state_) << " state";
            break;

        case Event::Type::kUnsubscribeRequested:
            LOG_WARNING() << "got kUnsubscribeRequested event when already unsubscribed";
            break;
    }
}

void Fsm::HandleOkReplyFromOtherServerId(ServerId other_server_id) {
    LOG_WARNING()
        << "Got a successful Subscribe reply from server_id=" << other_server_id.GetId()
        << " but current_server_id=" << current_server_id_.GetId()
        << ", rebalancing_server_id=" << rebalancing_server_id_.GetId() << ". Unsubscribe from it.";
    EmitAction(Action(Action::Type::kUnsubscribe, other_server_id));
}

void Fsm::HandleErrorReplyFromOtherServerId(ServerId other_server_id) {
    LOG_WARNING()
        << "Got an unsuccessful Subscribe reply from server_id=" << other_server_id.GetId() << " but current_server_id="
        << current_server_id_.GetId() << ", rebalancing_server_id=" << rebalancing_server_id_.GetId() << ". Ignore it.";
}

void Fsm::EmitAction(Action&& action) {
    // Every SUBSCRIBE attempt gets its own ID, even when retrying the same server.
    // UNSUBSCRIBE replies use the ID captured by the original SUBSCRIBE callback.
    if (action.type == Action::Type::kSubscribe) {
        action.subscribe_request_id = ++last_subscribe_request_id_;
    }
    LOG_INFO() << "Emitting action " << action.ToDebugString() << ", fsm=" << static_cast<void*>(this);
    pending_actions_.push_back(action);
}

const std::string& Fsm::StateToDebugString(State state) {
    static const std::string kSubscribing = "kSubscribing";
    static const std::string kSubscribed = "kSubscribed";
    static const std::string kUnsubscribing = "kUnsubscribing";
    static const std::string kRebalancingWaitSubscribe = "kRebalancingWaitSubscribe";
    static const std::string kRebalancingWaitUnsubscribe = "kRebalancingWaitUnsubscribe";
    static const std::string kUnsubscribed = "kUnsubscribed";

    switch (state) {
        case State::kSubscribing:
            return kSubscribing;

        case State::kSubscribed:
            return kSubscribed;

        case State::kUnsubscribing:
            return kUnsubscribing;

        case State::kRebalancingWaitSubscribe:
            return kRebalancingWaitSubscribe;

        case State::kRebalancingWaitUnsubscribe:
            return kRebalancingWaitUnsubscribe;

        case State::kUnsubscribed:
            return kUnsubscribed;
    }

    // not reachable
    static const std::string kNone;
    return kNone;
}

void Fsm::ChangeState(State new_state) {
    LOG_INFO()
        << "Fsm state switch: fsm=" << static_cast<void*>(this) << " " << StateToDebugString(state_) << " => "
        << StateToDebugString(new_state) << ", need_subscription=" << need_subscription_;
    state_ = new_state;
}

}  // namespace storages::redis::impl::shard_subscriber

USERVER_NAMESPACE_END
