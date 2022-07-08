#include "cluster_impl.hpp"

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

ClusterImpl::ClusterImpl(clients::dns::Resolver& resolver)
    : unreliable_{MonitoredPool{resolver, false}},
      reliable_{MonitoredPool{resolver, true}} {}

ChannelPtr ClusterImpl::GetUnreliable() {
  return unreliable_.GetPool()->Acquire();
}

ChannelPtr ClusterImpl::GetReliable() { return reliable_.GetPool()->Acquire(); }

ClusterImpl::MonitoredPool::MonitoredPool(clients::dns::Resolver& resolver,
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

ClusterImpl::MonitoredPool::~MonitoredPool() { monitor_.Stop(); }

std::shared_ptr<ChannelPool> ClusterImpl::MonitoredPool::GetPool() {
  return pool_.ReadCopy();
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
