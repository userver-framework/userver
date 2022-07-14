#include "shard_subscription_fsm.hpp"

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis::shard_subscriber {

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
  return "Event(" + TypeToDebugString(type) +
         ", server_id=" + std::to_string(server_id.GetId()) + ")";
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
  return "Action(" + TypeToDebugString(type) +
         ", server_id=" + std::to_string(server_id.GetId()) + ")";
}

Fsm::Fsm(size_t shard, ServerId server_id)
    : shard_(shard), current_server_id_(server_id) {
  // try to subscribe to anybody by default or to fixed server_id if specified
  EmitAction(Action(Action::Type::kSubscribe, server_id));
}

size_t Fsm::GetShard() const { return shard_; }

void Fsm::OnEvent(const Event& event) {
  LOG_DEBUG() << "OnEvent fsm=" << static_cast<void*>(this)
              << " state=" << StateToDebugString(state_)
              << " (current_server_id=" << current_server_id_.GetId() << ")"
              << " (rebalancing_server_id=" << rebalancing_server_id_.GetId()
              << ")"
              << " event=" << event.ToDebugString();

  switch (state_) {
    case State::kSubscribing:
      return HandleSubscribing(event);

    case State::kSubscribed:
      return HandleSubscribed(event);

    case State::kUnsubscribing:
      return HandleUnsubscribing(event);

    case State::kRebalancingWaitSubscribe:
      return HandleRebalancingWaitSubscribe(event);

    case State::kRebalancingWaitUnsubscribe:
      return HandleRebalancingWaitUnsubscribe(event);

    case State::kUnsubscribed:
      return HandleUnsubscribed(event);
  }
}

std::vector<Action> Fsm::PopAllPendingActions() {
  std::vector<Action> actions;
  swap(actions, pending_actions_);
  return actions;
}

ServerId Fsm::GetCurrentServerId() const { return current_server_id_; }

bool Fsm::CanBeRebalanced() const {
  return state_ == State::kSubscribed && need_subscription_;
}

std::chrono::steady_clock::time_point Fsm::GetCurrentServerTimePoint() const {
  return current_server_subscription_tp_;
}

void Fsm::SetNeedSubscription(bool need_subscription) {
  if (need_subscription_ == need_subscription) return;
  need_subscription_ = need_subscription;

  LOG_INFO() << "Fsm switch need_subscription: fsm=" << static_cast<void*>(this)
             << ", need_subscription=" << need_subscription_
             << ", state=" << StateToDebugString(state_);
}

void Fsm::HandleSubscribing(const Event& event) {
  if (!current_server_id_.IsAny()) {
    LOG_ERROR() << "current_server_id_ must be 'any' in this place. Buggy fsm?";
    return;
  }
  if (!rebalancing_server_id_.IsAny()) {
    LOG_ERROR()
        << "rebalancing_server_id_ must be 'any' in this place. Buggy fsm?";
    return;
  }

  switch (event.type) {
    case Event::Type::kSubscribeRequested:
      SetNeedSubscription(true);
      break;

    case Event::Type::kSubscribeReplyOk:
      current_server_subscription_tp_ = std::chrono::steady_clock::now();
      current_server_id_ = event.server_id;
      ChangeState(State::kSubscribed);
      LOG_DEBUG() << "Successfully subscribed to server_id="
                  << event.server_id.GetId();

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
        LOG_WARNING() << "Subscription to server_id=" << event.server_id.GetId()
                      << " failed. try any server_id";
        EmitAction(Action(Action::Type::kSubscribe, ServerId()));
      } else {
        ChangeState(State::kUnsubscribed);
        EmitAction(Action::Type::kDeleteFsm);
      }
      break;

    case Event::Type::kRebalanceRequested:
      LOG_WARNING() << "Ignore rebalance requests while "
                       "current_server_id_.IsAny()=true";
      break;

    case Event::Type::kUnsubscribeRequested:
      SetNeedSubscription(false);
      break;
  }
}

void Fsm::HandleSubscribed(const Event& event) {
  if (current_server_id_.IsAny()) {
    LOG_ERROR()
        << "current_server_id_ must not be 'any' in this place. Buggy fsm?";
    return;
  }
  if (!rebalancing_server_id_.IsAny()) {
    LOG_ERROR()
        << "rebalancing_server_id_ must be 'any' in this place. Buggy fsm?";
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
        LOG_WARNING() << "Duplicate OK subscribe reply from server_id="
                      << event.server_id.GetId();
        current_server_subscription_tp_ = std::chrono::steady_clock::now();
      }
      break;

    case Event::Type::kSubscribeReplyError:
      if (event.server_id == current_server_id_) {
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
        rebalancing_server_id_ = event.server_id;
        ChangeState(State::kRebalancingWaitSubscribe);
        EmitAction(Action(Action::Type::kSubscribe, rebalancing_server_id_));
      } else {
        LOG_WARNING() << "Requested rebalance to the same server_id="
                      << event.server_id.GetId();
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
    LOG_ERROR()
        << "current_server_id_ must not be 'any' in this place. Buggy fsm?";
    return;
  }
  if (!rebalancing_server_id_.IsAny()) {
    LOG_ERROR()
        << "rebalancing_server_id_ must be 'any' in this place. Buggy fsm?";
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
        LOG_WARNING() << "Duplicate OK subscribe reply from server_id="
                      << event.server_id.GetId();
        current_server_subscription_tp_ = std::chrono::steady_clock::now();
      }
      break;

    case Event::Type::kSubscribeReplyError:
      if (event.server_id == current_server_id_) {
        current_server_id_ = ServerId();
        if (need_subscription_) {
          // reset current instance, now we want to connect to any host
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
    LOG_ERROR()
        << "current_server_id_ must not be 'any' in this place. Buggy fsm?";
    return;
  }
  if (rebalancing_server_id_.IsAny()) {
    LOG_ERROR()
        << "rebalancing_server_id_ must not be 'any' in this place. Buggy fsm?";
    return;
  }

  switch (event.type) {
    case Event::Type::kSubscribeRequested:
      SetNeedSubscription(true);
      break;

    case Event::Type::kSubscribeReplyOk:
      if (event.server_id == current_server_id_) {
        LOG_WARNING() << "Got successful Subscribe reply from server_id="
                      << event.server_id.GetId()
                      << " while already subscribed to it";
      } else if (event.server_id == rebalancing_server_id_) {
        current_server_subscription_tp_ = std::chrono::steady_clock::now();
        LOG_DEBUG() << "Successfully subscribed to server_id="
                    << event.server_id.GetId() << " after rebalancing";
        std::swap(current_server_id_, rebalancing_server_id_);
        EmitAction(Action(Action::Type::kUnsubscribe, rebalancing_server_id_));
        ChangeState(State::kRebalancingWaitUnsubscribe);
        // we can't make any subscribe request until we have got some
        // 'unsubscribe' confirmation from rebalancing_server_id_
      } else {
        HandleOkReplyFromOtherServerId(event.server_id);
      }
      break;

    case Event::Type::kSubscribeReplyError:
      if (event.server_id == current_server_id_) {
        current_server_id_ = ServerId();
        rebalancing_server_id_ = ServerId();
        ChangeState(State::kSubscribing);
      } else if (event.server_id == rebalancing_server_id_) {
        LOG_WARNING() << "Rebalance subscription to server_id="
                      << rebalancing_server_id_.GetId() << " failed";
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
      LOG_INFO() << "Ignore rebalance request in " << StateToDebugString(state_)
                 << " state";
      break;

    case Event::Type::kUnsubscribeRequested:
      SetNeedSubscription(false);
      break;
  }
}

void Fsm::HandleRebalancingWaitUnsubscribe(const Event& event) {
  if (current_server_id_.IsAny()) {
    LOG_ERROR()
        << "current_server_id_ must not be 'any' in this place. Buggy fsm?";
    return;
  }
  if (rebalancing_server_id_.IsAny()) {
    LOG_ERROR()
        << "rebalancing_server_id_ must not be 'any' in this place. Buggy fsm?";
    return;
  }

  switch (event.type) {
    case Event::Type::kSubscribeRequested:
      SetNeedSubscription(true);
      break;

    case Event::Type::kSubscribeReplyOk:
      if (event.server_id == current_server_id_ ||
          event.server_id == rebalancing_server_id_) {
        LOG_WARNING() << "Got successful Subscribe reply from server_id="
                      << event.server_id.GetId()
                      << " while already subscribed to it";
      } else {
        HandleOkReplyFromOtherServerId(event.server_id);
      }
      break;

    case Event::Type::kSubscribeReplyError:
      if (event.server_id == current_server_id_) {
        current_server_id_ = rebalancing_server_id_;
        rebalancing_server_id_ = ServerId();
        ChangeState(State::kUnsubscribing);
        // Still waiting for 'unsubscribe' confirmation. Now from
        // current_server_id_.
      } else if (event.server_id == rebalancing_server_id_) {
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
      LOG_INFO() << "Ignore rebalance request in " << StateToDebugString(state_)
                 << " state";
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
    LOG_ERROR()
        << "rebalancing_server_id_ must be 'any' in this place. Buggy fsm?";
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
      LOG_INFO() << "Ignore rebalance request in " << StateToDebugString(state_)
                 << " state";
      break;

    case Event::Type::kUnsubscribeRequested:
      LOG_WARNING()
          << "got kUnsubscribeRequested event when already unsubscribed";
      break;
  }
}

void Fsm::HandleOkReplyFromOtherServerId(ServerId other_server_id) {
  LOG_WARNING() << "Got a successful Subscribe reply from server_id="
                << other_server_id.GetId()
                << " but current_server_id=" << current_server_id_.GetId()
                << ", rebalancing_server_id=" << rebalancing_server_id_.GetId()
                << ". Unsubscribe from it.";
  EmitAction(Action(Action::Type::kUnsubscribe, other_server_id));
}

void Fsm::HandleErrorReplyFromOtherServerId(ServerId other_server_id) {
  LOG_WARNING() << "Got an unsuccessful Subscribe reply from server_id="
                << other_server_id.GetId()
                << " but current_server_id=" << current_server_id_.GetId()
                << ", rebalancing_server_id=" << rebalancing_server_id_.GetId()
                << ". Ignore it.";
}

void Fsm::EmitAction(Action&& action) {
  LOG_INFO() << "Emitting action " << action.ToDebugString()
             << ", fsm=" << static_cast<void*>(this);
  pending_actions_.push_back(action);
}

const std::string& Fsm::StateToDebugString(State state) {
  static const std::string subscribing = "kSubscribing";
  static const std::string subscribed = "kSubscribed";
  static const std::string unsubscribing = "kUnsubscribing";
  static const std::string rebalancing_wait_subscribe =
      "kRebalancingWaitSubscribe";
  static const std::string rebalancing_wait_unsubscribe =
      "kRebalancingWaitUnsubscribe";
  static const std::string unsubscribed = "kUnsubscribed";

  switch (state) {
    case State::kSubscribing:
      return subscribing;

    case State::kSubscribed:
      return subscribed;

    case State::kUnsubscribing:
      return unsubscribing;

    case State::kRebalancingWaitSubscribe:
      return rebalancing_wait_subscribe;

    case State::kRebalancingWaitUnsubscribe:
      return rebalancing_wait_unsubscribe;

    case State::kUnsubscribed:
      return unsubscribed;
  }

  // not reachable
  static const std::string none;
  return none;
}

void Fsm::ChangeState(State new_state) {
  LOG_INFO() << "Fsm state switch: fsm=" << static_cast<void*>(this) << " "
             << StateToDebugString(state_) << " => "
             << StateToDebugString(new_state)
             << ", need_subscription=" << need_subscription_;
  state_ = new_state;
}

}  // namespace redis::shard_subscriber

USERVER_NAMESPACE_END
