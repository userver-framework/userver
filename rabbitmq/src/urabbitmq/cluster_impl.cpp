#include "cluster_impl.hpp"

#include <urabbitmq/channel_pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

namespace {

std::shared_ptr<ChannelPool> CreatePool(clients::dns::Resolver& resolver,
                                        bool reliable) {
  const ChannelPoolSettings settings{
      reliable ? ChannelPoolMode::kReliable : ChannelPoolMode::kUnreliable, 10};

  return std::make_shared<ChannelPool>(resolver, settings);
}

}  // namespace

ClusterImpl::ClusterImpl(clients::dns::Resolver& resolver)
    : unreliable_{CreatePool(resolver, false)},
      reliable_{CreatePool(resolver, true)} {}

ChannelPtr ClusterImpl::GetUnreliable() { return unreliable_->Acquire(); }

ChannelPtr ClusterImpl::GetReliable() { return reliable_->Acquire(); }

}  // namespace urabbitmq

USERVER_NAMESPACE_END
