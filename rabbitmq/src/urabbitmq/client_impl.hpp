#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <userver/urabbitmq/client_settings.hpp>

#include <engine/ev/thread_pool.hpp>
#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/formats/json_fwd.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/periodic_task.hpp>

#include <urabbitmq/channel_ptr.hpp>
#include <urabbitmq/connection_settings.hpp>
#include <urabbitmq/statistics/connection_statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class Connection;

class ClientImpl final {
 public:
  ClientImpl(clients::dns::Resolver& resolver, const ClientSettings& settings);

  ChannelPtr GetUnreliable();

  ChannelPtr GetReliable();

  formats::json::Value GetStatistics() const;

 private:
  // This class monitors the connection, reestablishing it if necessary.
  // We allow initial connect routine to fail, otherwise service won't be able
  // to start in case of cluster maintenance/outage.
  class MonitoredConnection final {
   public:
    MonitoredConnection(clients::dns::Resolver& resolver,
                        engine::ev::ThreadControl& thread,
                        const ConnectionSettings& connection_settings,
                        const EndpointInfo& endpoint,
                        const AuthSettings& auth_settings);
    ~MonitoredConnection();

    ChannelPtr Acquire();
    ChannelPtr AcquireReliable();

    statistics::ConnectionStatistics::Frozen GetStatistics() const;

    bool IsBroken();

   private:
    void EnsureNotBroken();

    clients::dns::Resolver& resolver_;
    engine::ev::ThreadControl& ev_thread_;

    const ConnectionSettings connection_settings_;
    const EndpointInfo endpoint_;
    const AuthSettings auth_settings_;

    statistics::ConnectionStatistics stats_;

    rcu::Variable<std::shared_ptr<Connection>> connection_;
    utils::PeriodicTask monitor_;
  };

  // We use this to initialize a vector of atomics
  struct CopyableAtomic final {
    CopyableAtomic();
    CopyableAtomic(const CopyableAtomic& other);
    CopyableAtomic& operator=(const CopyableAtomic& other);

    std::atomic<size_t> atomic{0};
  };

  engine::ev::ThreadControl& GetNextEvThread() const;

  std::size_t CalculateConnectionsCountPerHost() const;

  MonitoredConnection& GetNextConnection();

  const ClientSettings settings_;
  std::unique_ptr<engine::ev::ThreadPool> owned_ev_pool_;
  const size_t connections_per_host_;

  std::vector<std::unique_ptr<MonitoredConnection>> connections_;
  std::vector<CopyableAtomic> host_conn_idx_{};
  std::atomic<size_t> host_idx_{0};
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
