#pragma once

#include <vector>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/periodic_task.hpp>

#include <urabbitmq/channel_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class ClientImpl final {
 public:
  ClientImpl(clients::dns::Resolver& resolver);

  ChannelPtr GetUnreliable();

  ChannelPtr GetReliable();

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

  ChannelPtr GetChannel(std::vector<std::unique_ptr<MonitoredPool>>& pools,
                        std::atomic<size_t>& idx);

  std::atomic<size_t> unreliable_idx_{0};
  std::vector<std::unique_ptr<MonitoredPool>> unreliable_;
  std::atomic<size_t> reliable_idx_{0};
  std::vector<std::unique_ptr<MonitoredPool>> reliable_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
