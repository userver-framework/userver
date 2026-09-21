#include "subscription_rebalance_scheduler.hpp"

#include <userver/logging/log.hpp>

#include "redis.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

const std::chrono::seconds kRebalanceMinIntervalDefault{30};

SubscriptionRebalanceScheduler::SubscriptionRebalanceScheduler(
    const engine::ev::ThreadControl& thread_control,
    SubscriptionStorageBase& storage,
    size_t shard_idx
)
    : thread_control_(thread_control),
      storage_(storage),
      shard_idx_(shard_idx),
      rebalance_queue_(RebalanceQueue::Create()),
      rebalance_producer_(rebalance_queue_->GetMultiProducer()),
      rebalance_consumer_(rebalance_queue_->GetConsumer()),
      rebalance_min_interval_ms_(std::chrono::duration_cast<std::chrono::milliseconds>(kRebalanceMinIntervalDefault)
                                     .count())
{
    timer_.data = this;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_timer_init(&timer_, OnTimer, 0.0, 0.0);

    rebalance_request_watcher_.data = this;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_async_init(&rebalance_request_watcher_, OnRebalanceRequested);
    thread_control_.RunInEvLoopSyncWithResult([this] { thread_control_.Start(rebalance_request_watcher_); });
}

SubscriptionRebalanceScheduler::~SubscriptionRebalanceScheduler() { Stop(); }

void SubscriptionRebalanceScheduler::RequestRebalance(ServerWeights weights) {
    if (stopped_.load(std::memory_order_acquire) || !rebalance_producer_.PushNoblock(std::move(weights))) {
        return;
    }
    if (!rebalance_scheduled_.exchange(true, std::memory_order_acq_rel)) {
        thread_control_.Send(rebalance_request_watcher_);
    }
}

void SubscriptionRebalanceScheduler::Stop() {
    if (stopped_.exchange(true, std::memory_order_acq_rel)) {
        return;
    }
    thread_control_.RunInEvLoopSyncWithResult([this] {
        thread_control_.Stop(rebalance_request_watcher_);
        thread_control_.Stop(timer_);
    });
}

void SubscriptionRebalanceScheduler::SetRebalanceMinInterval(std::chrono::milliseconds interval) {
    rebalance_min_interval_ms_.store(interval.count(), std::memory_order_relaxed);
}

std::chrono::milliseconds SubscriptionRebalanceScheduler::GetRebalanceMinInterval() const {
    return std::chrono::milliseconds{rebalance_min_interval_ms_.load(std::memory_order_relaxed)};
}

bool SubscriptionRebalanceScheduler::TryTakeLatestWeights(ServerWeights& weights) {
    ServerWeights next_weights;
    bool has_weights = false;
    while (rebalance_consumer_.PopNoblock(next_weights)) {
        weights = std::move(next_weights);
        has_weights = true;
    }
    return has_weights;
}

void SubscriptionRebalanceScheduler::DoRebalance() {
    ServerWeights weights;
    if (!TryTakeLatestWeights(weights) || weights.empty()) {
        rebalance_scheduled_.store(false, std::memory_order_release);
        if (!TryTakeLatestWeights(weights) || weights.empty()) {
            return;
        }
        rebalance_scheduled_.exchange(true, std::memory_order_acq_rel);
    }
    try {
        storage_.DoRebalance(shard_idx_, std::move(weights));
    } catch (const std::exception& ex) {
        LOG_ERROR() << "exception in DoRebalance(shard_idx=" << shard_idx_ << "): " << ex.what();
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_timer_set(&timer_, ToEvDuration(GetRebalanceMinInterval()), 0.0);
    thread_control_.Start(timer_);
}

void SubscriptionRebalanceScheduler::OnRebalanceRequested(struct ev_loop*, ev_async* w, int) noexcept {
    auto* scheduler = static_cast<SubscriptionRebalanceScheduler*>(w->data);
    scheduler->DoRebalance();
}

void SubscriptionRebalanceScheduler::OnTimer(struct ev_loop*, ev_timer* w, int) noexcept {
    auto* scheduler = static_cast<SubscriptionRebalanceScheduler*>(w->data);
    scheduler->DoRebalance();
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
