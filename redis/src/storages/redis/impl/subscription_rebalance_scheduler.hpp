#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#include <engine/ev/thread_control.hpp>
#include <engine/ev/thread_pool.hpp>
#include <userver/concurrent/mpsc_queue.hpp>

#include <userver/storages/redis/base.hpp>
#include "subscription_storage.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

class SubscriptionRebalanceScheduler {
public:
    SubscriptionRebalanceScheduler(
        const engine::ev::ThreadControl& thread_control,
        SubscriptionStorageBase& storage,
        size_t shard_idx
    );
    ~SubscriptionRebalanceScheduler();

    using ServerWeights = SubscriptionStorageBase::ServerWeights;

    void RequestRebalance(ServerWeights weights);

    void Stop();

    void SetRebalanceMinInterval(std::chrono::milliseconds interval);

    std::chrono::milliseconds GetRebalanceMinInterval() const;

private:
    using RebalanceQueue = concurrent::MpscQueue<ServerWeights>;

    void DoRebalance();
    bool TryTakeLatestWeights(ServerWeights& weights);

    static void OnRebalanceRequested(struct ev_loop*, ev_async* w, int) noexcept;
    static void OnTimer(struct ev_loop*, ev_timer* w, int) noexcept;

    engine::ev::ThreadControl thread_control_;
    SubscriptionStorageBase& storage_;
    const size_t shard_idx_;

    ev_timer timer_{};
    ev_async rebalance_request_watcher_{};

    std::shared_ptr<RebalanceQueue> rebalance_queue_;
    RebalanceQueue::MultiProducer rebalance_producer_;
    RebalanceQueue::Consumer rebalance_consumer_;
    std::atomic<bool> rebalance_scheduled_{false};
    std::atomic<bool> stopped_{false};
    std::atomic<std::int64_t> rebalance_min_interval_ms_;
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
