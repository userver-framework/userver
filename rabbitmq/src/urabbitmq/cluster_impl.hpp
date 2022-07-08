#pragma once

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/periodic_task.hpp>

#include <urabbitmq/channel_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class ClusterImpl final {
 public:
  ClusterImpl(clients::dns::Resolver& resolver);

  ChannelPtr GetUnreliable();

  ChannelPtr GetReliable();

  void Reset();

 private:
  class MonitoredPool final {
   public:
    MonitoredPool(clients::dns::Resolver& resolver, bool reliable);
    ~MonitoredPool();

    std::shared_ptr<ChannelPool> GetPool();

   private:
    clients::dns::Resolver& resolver_;
    bool reliable_;
    rcu::Variable<std::shared_ptr<ChannelPool>> pool_;
    utils::PeriodicTask monitor_;
  };

  MonitoredPool unreliable_;
  MonitoredPool reliable_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
