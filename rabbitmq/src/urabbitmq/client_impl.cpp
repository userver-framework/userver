#include "client_impl.hpp"

#include <userver/engine/async.hpp>
#include <userver/engine/wait_all_checked.hpp>

#include <urabbitmq/channel_pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

namespace {

std::shared_ptr<ChannelPool> CreatePoolPtr(clients::dns::Resolver& resolver,
                                           bool reliable) {
  const ChannelPoolSettings settings{
      reliable ? ChannelPoolMode::kReliable : ChannelPoolMode::kUnreliable, 10};

  return std::make_shared<ChannelPool>(resolver, settings);
}

}  // namespace

ClientImpl::ClientImpl(clients::dns::Resolver& resolver) {
  constexpr size_t pools_count = 3;
  unreliable_.resize(pools_count);
  reliable_.resize(pools_count);

  std::vector<engine::TaskWithResult<void>> init_tasks;
  init_tasks.reserve(pools_count * 2);

  for (size_t i = 0; i < pools_count * 2; ++i) {
    init_tasks.emplace_back(engine::AsyncNoSpan(
        [this, ind = i % pools_count, reliable = i >= pools_count, &resolver] {
          auto& pool_ptr = reliable ? reliable_[ind] : unreliable_[ind];
          pool_ptr = std::make_unique<MonitoredPool>(resolver, reliable);
        }));
  }
  engine::WaitAllChecked(init_tasks);
}

ChannelPtr ClientImpl::GetUnreliable() {
  return GetChannel(unreliable_, unreliable_idx_);
}

ChannelPtr ClientImpl::GetReliable() {
  return GetChannel(reliable_, reliable_idx_);
}

ClientImpl::MonitoredPool::MonitoredPool(clients::dns::Resolver& resolver,
                                         bool reliable)
    : resolver_{resolver},
      reliable_{reliable},
      pool_{CreatePoolPtr(resolver_, reliable_)} {
  monitor_.Start("cluster_monitor", {std::chrono::milliseconds{1000}}, [this] {
    if (GetPool()->IsBroken()) {
      pool_.Emplace(CreatePoolPtr(resolver_, reliable_));
    }
  });
}

ClientImpl::MonitoredPool::~MonitoredPool() { monitor_.Stop(); }

std::shared_ptr<ChannelPool> ClientImpl::MonitoredPool::GetPool() {
  return pool_.ReadCopy();
}

ChannelPtr ClientImpl::GetChannel(
    std::vector<std::unique_ptr<MonitoredPool>>& pools,
    std::atomic<size_t>& idx) {
  return pools[idx++ % pools.size()]->GetPool()->Acquire();
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
