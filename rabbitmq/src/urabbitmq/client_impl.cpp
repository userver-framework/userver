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
    size_t max_channels, bool reliable) {
  const ConnectionSettings settings{
      reliable ? ConnectionMode::kReliable : ConnectionMode::kUnreliable,
      max_channels};

  return std::make_shared<Connection>(resolver, thread, settings);
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
  const auto connections_count = (settings.ev_pool_type == EvPoolType::kOwned
                                      ? settings.thread_count
                                      : engine::current_task::GetTaskProcessor()
                                            .EventThreadPool()
                                            .GetSize()) *
                                 settings.connections_per_thread;
  unreliable_.connections.resize(connections_count);
  reliable_.connections.resize(connections_count);

  std::vector<engine::TaskWithResult<void>> init_tasks;
  init_tasks.reserve(connections_count * 2);

  for (size_t i = 0; i < connections_count; ++i) {
    init_tasks.emplace_back(engine::AsyncNoSpan([this, &resolver, &settings,
                                                 i] {
      unreliable_.connections[i] = std::make_unique<MonitoredConnection>(
          resolver, GetNextEvThread(), settings.channels_per_connection, false);
    }));
    init_tasks.emplace_back(engine::AsyncNoSpan([this, &resolver, &settings,
                                                 i] {
      reliable_.connections[i] = std::make_unique<MonitoredConnection>(
          resolver, GetNextEvThread(), settings.channels_per_connection, true);
    }));
  }
  engine::WaitAllChecked(init_tasks);
}

ChannelPtr ClientImpl::GetUnreliable() { return unreliable_.GetChannel(); }

ChannelPtr ClientImpl::GetReliable() { return reliable_.GetChannel(); }

ClientImpl::MonitoredConnection::MonitoredConnection(
    clients::dns::Resolver& resolver, engine::ev::ThreadControl& thread,
    size_t max_channels, bool reliable)
    : resolver_{resolver},
      ev_thread_{thread},
      max_channels_{max_channels},
      reliable_{reliable},
      connection_{CreateConnectionPtr(resolver_, ev_thread_, max_channels_,
                                      reliable_)} {
  monitor_.Start("cluster_monitor", {std::chrono::milliseconds{1000}}, [this] {
    if (GetConnection()->IsBroken()) {
      connection_.Emplace(
          CreateConnectionPtr(resolver_, ev_thread_, max_channels_, reliable_));
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

engine::ev::ThreadControl& ClientImpl::GetNextEvThread() const {
  return owned_ev_pool_ == nullptr ? engine::current_task::GetEventThread()
                                   : owned_ev_pool_->NextThread();
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
