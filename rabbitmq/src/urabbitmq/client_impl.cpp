#include "client_impl.hpp"

#include <engine/ev/thread_pool.hpp>
#include <engine/task/task_processor.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/wait_all_checked.hpp>

#include <userver/urabbitmq/client_settings.hpp>

#include <urabbitmq/connection.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

namespace {

std::shared_ptr<Connection> CreateConnectionPtr(
    clients::dns::Resolver& resolver, engine::ev::ThreadControl& thread,
    size_t max_channels, bool reliable, const EndpointInfo& endpoint,
    const AuthSettings& auth_settings) {
  const ConnectionSettings settings{
      reliable ? ConnectionMode::kReliable : ConnectionMode::kUnreliable,
      max_channels};

  return std::make_shared<Connection>(resolver, thread, settings, endpoint,
                                      auth_settings);
}

std::unique_ptr<engine::ev::ThreadPool> CreateEvThreadPool(
    const ClientSettings& settings) {
  if (settings.ev_pool_type == EvPoolType::kShared) return nullptr;

  const engine::ev::ThreadPoolConfig config{settings.thread_count,
                                            "urmq-worker", true, true};
  return std::make_unique<engine::ev::ThreadPool>(config);
}

}  // namespace

ClientImpl::ClientImpl(clients::dns::Resolver& resolver,
                       const ClientSettings& settings)
    : owned_ev_pool_{CreateEvThreadPool(settings)} {
  UINVARIANT(!settings.endpoints.empty(), "empty set of hosts");
  hosts_.hosts.resize(settings.endpoints.size());

  std::vector<engine::TaskWithResult<void>> init_tasks;
  for (size_t i = 0; i < settings.endpoints.size(); ++i) {
    init_tasks.emplace_back(
        engine::AsyncNoSpan([this, &resolver, &settings, i] {
          hosts_.hosts[i] = std::make_unique<Host>(
              *this, resolver, settings.endpoints[i], settings);
        }));
  }
  engine::WaitAllChecked(init_tasks);
}

ChannelPtr ClientImpl::GetUnreliable() { return hosts_.GetUnreliable(); }

ChannelPtr ClientImpl::GetReliable() { return hosts_.GetReliable(); }

ClientImpl::MonitoredConnection::MonitoredConnection(
    clients::dns::Resolver& resolver, engine::ev::ThreadControl& thread,
    size_t max_channels, bool reliable, const EndpointInfo& endpoint,
    const AuthSettings& auth_settings)
    : resolver_{resolver},
      ev_thread_{thread},
      endpoint_{endpoint},
      auth_settings_{auth_settings},
      max_channels_{max_channels},
      reliable_{reliable},
      connection_{CreateConnectionPtr(resolver_, ev_thread_, max_channels_,
                                      reliable_, endpoint_, auth_settings_)} {
  monitor_.Start(
      "connection_monitor", {std::chrono::milliseconds{1000}}, [this] {
        if (GetConnection()->IsBroken()) {
          connection_.Emplace(CreateConnectionPtr(resolver_, ev_thread_,
                                                  max_channels_, reliable_,
                                                  endpoint_, auth_settings_));
        }
      });
}

ClientImpl::MonitoredConnection::~MonitoredConnection() { monitor_.Stop(); }

std::shared_ptr<Connection> ClientImpl::MonitoredConnection::GetConnection() {
  return connection_.ReadCopy();
}

ChannelPtr ClientImpl::ConnectionPool::GetChannel() {
  return connections[idx++ % connections.size()]->GetConnection()->Acquire();
}

ClientImpl::Host::Host(ClientImpl& parent, clients::dns::Resolver& resolver,
                       const EndpointInfo& endpoint,
                       const ClientSettings& settings) {
  const auto connections_count = (settings.ev_pool_type == EvPoolType::kOwned
                                      ? settings.thread_count
                                      : engine::current_task::GetTaskProcessor()
                                            .EventThreadPool()
                                            .GetSize()) *
                                 settings.connections_per_thread;
  unreliable.connections.resize(connections_count);
  reliable.connections.resize(connections_count);

  std::vector<engine::TaskWithResult<void>> init_tasks;
  init_tasks.reserve(connections_count * 2);

  for (size_t i = 0; i < connections_count; ++i) {
    init_tasks.emplace_back(engine::AsyncNoSpan(
        [this, &parent, &resolver, &endpoint, &settings, i] {
          unreliable.connections[i] = std::make_unique<MonitoredConnection>(
              resolver, parent.GetNextEvThread(),
              settings.channels_per_connection, false, endpoint, settings.auth);
        }));
    init_tasks.emplace_back(engine::AsyncNoSpan(
        [this, &parent, &resolver, &endpoint, &settings, i] {
          reliable.connections[i] = std::make_unique<MonitoredConnection>(
              resolver, parent.GetNextEvThread(),
              settings.channels_per_connection, true, endpoint, settings.auth);
        }));
  }
  engine::WaitAllChecked(init_tasks);
}

ChannelPtr ClientImpl::HostPool::GetUnreliable() {
  return hosts[idx++ % hosts.size()]->unreliable.GetChannel();
}

ChannelPtr ClientImpl::HostPool::GetReliable() {
  return hosts[idx++ % hosts.size()]->reliable.GetChannel();
}

engine::ev::ThreadControl& ClientImpl::GetNextEvThread() const {
  return owned_ev_pool_ == nullptr ? engine::current_task::GetEventThread()
                                   : owned_ev_pool_->NextThread();
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
