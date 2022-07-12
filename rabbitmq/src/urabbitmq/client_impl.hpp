#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <userver/urabbitmq/client_settings.hpp>

#include <engine/ev/thread_pool.hpp>
#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/periodic_task.hpp>

#include <urabbitmq/channel_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class Connection;

class ClientImpl final {
 public:
  ClientImpl(clients::dns::Resolver& resolver, const ClientSettings& settings);

  ChannelPtr GetUnreliable();

  ChannelPtr GetReliable();

 private:
  class MonitoredConnection final {
   public:
    MonitoredConnection(clients::dns::Resolver& resolver,
                        engine::ev::ThreadControl& thread, size_t max_channels,
                        bool reliable, const EndpointInfo& endpoint,
                        const AuthSettings& auth_settings);
    ~MonitoredConnection();

    std::shared_ptr<Connection> GetConnection();

   private:
    clients::dns::Resolver& resolver_;
    engine::ev::ThreadControl& ev_thread_;

    const EndpointInfo endpoint_;
    const AuthSettings auth_settings_;

    size_t max_channels_;
    bool reliable_;

    rcu::Variable<std::shared_ptr<Connection>> connection_;
    utils::PeriodicTask monitor_;
  };

  struct ConnectionPool final {
    std::atomic<size_t> idx{0};
    std::vector<std::unique_ptr<MonitoredConnection>> connections{};

    ChannelPtr GetChannel();
  };

  struct Host final {
    Host(ClientImpl& parent, clients::dns::Resolver& resolver,
         const EndpointInfo& endpoint, const ClientSettings& settings);

    ConnectionPool unreliable;
    ConnectionPool reliable;
  };
  friend struct Host;

  struct HostPool final {
    std::atomic<size_t> idx{0};
    std::vector<std::unique_ptr<Host>> hosts{};

    ChannelPtr GetReliable();

    ChannelPtr GetUnreliable();
  };
  // TODO : reduce indirection, it's ridiculous right now :)

  engine::ev::ThreadControl& GetNextEvThread() const;

  std::unique_ptr<engine::ev::ThreadPool> owned_ev_pool_;
  HostPool hosts_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
