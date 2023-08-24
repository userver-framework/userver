#pragma once

#include <userver/storages/redis/impl/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis::shard_subscriber {

struct Event {
  enum class Type {
    /* Got subscribe request */
    kSubscribeRequested,

    /* Server returned OK to (p)subscribe */
    kSubscribeReplyOk,

    /* One of three events: */
    /* 1. Got UNSUBSCRIBE reply to (p)subscribe command */
    /* 3. Got error reply to (p)subscribe command due to connection close */
    /* 3. Got error reply to (p)subscribe command due to no alive hosts found */
    kSubscribeReplyError,

    /* Got rebalance request with new server_id */
    kRebalanceRequested,

    /* Got unsubscribe request */
    kUnsubscribeRequested
  };

  static std::string TypeToDebugString(Type type);
  std::string ToDebugString() const;

  Type type{Type::kSubscribeRequested};
  ServerId server_id;
};

struct Action {
  enum class Type {
    // Subscribe to a channel via server_id
    kSubscribe,

    // Unsubscribe from a channel via server_id
    kUnsubscribe,

    // Fsm is not subscribed to a channel,
    // has no active subscribe/unsubscribe requests, and
    // does not need to be subscribed. So it can be deleted.
    kDeleteFsm
  };

  Action(Type type, ServerId server_id = ServerId())
      : type(type), server_id(server_id) {}

  static std::string TypeToDebugString(Type type);
  std::string ToDebugString() const;

  Type type;
  ServerId server_id;
};

class Fsm {
 public:
  explicit Fsm(size_t shard, ServerId server_id = {});

  size_t GetShard() const;

  void OnEvent(const Event& event);

  std::vector<Action> PopAllPendingActions();

  ServerId GetCurrentServerId() const;
  bool CanBeRebalanced() const;

  std::chrono::steady_clock::time_point GetCurrentServerTimePoint() const;

 private:
  enum class State {
    /* We've sent subscribe command, waiting the reply */
    kSubscribing,

    /* We're subscribed */
    kSubscribed,

    /* We've sent unsubscribe command with current_server_id, waiting the
       reply */
    kUnsubscribing,

    /* We've sent subscribe command with rebalancing_server_id, waiting the
       reply */
    kRebalancingWaitSubscribe,

    /* We've sent unsubscribe command with rebalancing_server_id, waiting the
       reply */
    kRebalancingWaitUnsubscribe,

    /* We're unsubscribed */
    kUnsubscribed
  };

  void SetNeedSubscription(bool need_subscription);

  void HandleSubscribing(const Event& event);
  void HandleSubscribed(const Event& event);
  void HandleUnsubscribing(const Event& event);
  void HandleRebalancingWaitSubscribe(const Event& event);
  void HandleRebalancingWaitUnsubscribe(const Event& event);
  void HandleUnsubscribed(const Event& event);

  void HandleOkReplyFromOtherServerId(ServerId other_server_id);
  void HandleErrorReplyFromOtherServerId(ServerId other_server_id);

  void EmitAction(Action&& action);

  static const std::string& StateToDebugString(State state);
  void ChangeState(State new_state);

  State state_{State::kSubscribing};
  const size_t shard_;

  bool need_subscription_{true};

  // we're either subscribed on this id or sent a subscription request
  ServerId current_server_id_;
  ServerId rebalancing_server_id_;

  std::chrono::steady_clock::time_point current_server_subscription_tp_;

  std::vector<Action> pending_actions_;
};

}  // namespace redis::shard_subscriber

USERVER_NAMESPACE_END
