#include "subscription_rebalance_scheduler.hpp"

#include <userver/logging/log.hpp>

#include "redis.hpp"

USERVER_NAMESPACE_BEGIN

namespace redis {

const std::chrono::seconds kRebalanceMinIntervalDefault{30};

SubscriptionRebalanceScheduler::SubscriptionRebalanceScheduler(
    engine::ev::ThreadPool& thread_pool, SubscriptionStorage& storage,
    size_t shard_idx)
    : thread_control_(thread_pool.NextThread()),
      storage_(storage),
      shard_idx_(shard_idx),
      rebalance_min_interval_{kRebalanceMinIntervalDefault} {
  timer_.data = this;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_timer_init(&timer_, OnTimer, 0.0, 0.0);

  rebalance_request_watcher_.data = this;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_async_init(&rebalance_request_watcher_, OnRebalanceRequested);
  thread_control_.RunInEvLoopBlocking(
      [this] { thread_control_.Start(rebalance_request_watcher_); });
}

SubscriptionRebalanceScheduler::~SubscriptionRebalanceScheduler() { Stop(); }

void SubscriptionRebalanceScheduler::RequestRebalance(ServerWeights weights) {
  bool need_notify = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    weights_ = std::move(weights);
    if (!next_rebalance_scheduled_) {
      next_rebalance_scheduled_ = true;
      need_notify = true;
    }
  }
  if (need_notify) {
    thread_control_.Send(rebalance_request_watcher_);
  }
}

void SubscriptionRebalanceScheduler::Stop() {
  thread_control_.RunInEvLoopBlocking([this] {
    thread_control_.Stop(rebalance_request_watcher_);
    thread_control_.Stop(timer_);
  });
}

void SubscriptionRebalanceScheduler::SetRebalanceMinInterval(
    std::chrono::milliseconds interval) {
  std::lock_guard<std::mutex> lock(mutex_);
  rebalance_min_interval_ = interval;
}

std::chrono::milliseconds
SubscriptionRebalanceScheduler::GetRebalanceMinInterval() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return rebalance_min_interval_;
}

void SubscriptionRebalanceScheduler::DoRebalance() {
  ServerWeights weights;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    weights.swap(weights_);
    if (weights.empty()) {
      next_rebalance_scheduled_ = false;
      return;
    }
  }
  try {
    storage_.DoRebalance(shard_idx_, std::move(weights));
  } catch (const std::exception& ex) {
    LOG_ERROR() << "exception in DoRebalance(shard_idx=" << shard_idx_
                << "): " << ex.what();
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_timer_set(&timer_, ToEvDuration(GetRebalanceMinInterval()), 0.0);
  thread_control_.Start(timer_);
}

void SubscriptionRebalanceScheduler::OnRebalanceRequested(struct ev_loop*,
                                                          ev_async* w,
                                                          int) noexcept {
  auto* scheduler = static_cast<SubscriptionRebalanceScheduler*>(w->data);
  scheduler->DoRebalance();
}

void SubscriptionRebalanceScheduler::OnTimer(struct ev_loop*, ev_timer* w,
                                             int) noexcept {
  auto* scheduler = static_cast<SubscriptionRebalanceScheduler*>(w->data);
  scheduler->DoRebalance();
}

}  // namespace redis

USERVER_NAMESPACE_END
