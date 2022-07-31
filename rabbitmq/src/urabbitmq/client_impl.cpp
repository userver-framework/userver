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
    const ConnectionSettings& connection_settings, const EndpointInfo& endpoint,
    const AuthSettings& auth_settings,
    statistics::ConnectionStatistics& stats) {
  return std::make_shared<Connection>(resolver, thread, endpoint, auth_settings,
                                      connection_settings, stats);
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
    : settings_{settings},
      owned_ev_pool_{CreateEvThreadPool(settings)},
      connections_per_host_{CalculateConnectionsCountPerHost()} {
  connections_.resize(connections_per_host_ *
                      settings_.endpoints.endpoints.size());
  host_conn_idx_.assign(settings_.endpoints.endpoints.size(), CopyableAtomic{});

  std::vector<engine::TaskWithResult<void>> init_tasks;
  init_tasks.reserve(connections_.size());

  const ConnectionSettings connection_settings{
      settings_.channels_per_connection, settings_.secure};
  for (size_t i = 0; i < connections_.size(); ++i) {
    init_tasks.emplace_back(
        engine::AsyncNoSpan([this, &resolver, &connection_settings, i] {
          connections_[i] = std::make_unique<MonitoredConnection>(
              resolver, GetNextEvThread(), connection_settings,
              settings_.endpoints.endpoints[i / connections_per_host_],
              settings_.endpoints.auth);
        }));
  }
  engine::WaitAllChecked(init_tasks);
}

ChannelPtr ClientImpl::GetUnreliable() { return GetNextConnection().Acquire(); }

ChannelPtr ClientImpl::GetReliable() {
  return GetNextConnection().AcquireReliable();
}

formats::json::Value ClientImpl::GetStatistics() const {
  formats::json::ValueBuilder builder{formats::json::Type::kObject};

  size_t conn_ind = 0;
  for (const auto& endpoint : settings_.endpoints.endpoints) {
    const auto& host = endpoint.host;
    statistics::ConnectionStatistics::Frozen host_stats{};
    for (size_t i = 0; i < connections_per_host_; ++i, ++conn_ind) {
      host_stats += connections_[i]->GetStatistics();
    }
    builder[host] = host_stats;
  }

  return builder.ExtractValue();
}

ClientImpl::MonitoredConnection::MonitoredConnection(
    clients::dns::Resolver& resolver, engine::ev::ThreadControl& thread,
    const ConnectionSettings& connection_settings, const EndpointInfo& endpoint,
    const AuthSettings& auth_settings)
    : resolver_{resolver},
      ev_thread_{thread},
      connection_settings_{connection_settings},
      endpoint_{endpoint},
      auth_settings_{auth_settings},
      connection_{CreateConnectionPtr(resolver_, ev_thread_,
                                      connection_settings_, endpoint_,
                                      auth_settings_, stats_)} {
  monitor_.Start(
      "connection_monitor", {std::chrono::milliseconds{1000}}, [this] {
        if (IsBroken()) {
          try {
            connection_.Emplace(
                CreateConnectionPtr(resolver_, ev_thread_, connection_settings_,
                                    endpoint_, auth_settings_, stats_));
            stats_.AccountReconnectSuccess();
          } catch (const std::exception& ex) {
            stats_.AccountReconnectFailure();
            LOG_ERROR() << "Failed to recreate a connection: '" << ex.what()
                        << "', will retry";
          }
        }
      });
}

ClientImpl::MonitoredConnection::~MonitoredConnection() { monitor_.Stop(); }

ChannelPtr ClientImpl::MonitoredConnection::Acquire() {
  auto readable = connection_.Read();
  return (*readable)->Acquire();
}

ChannelPtr ClientImpl::MonitoredConnection::AcquireReliable() {
  auto readable = connection_.Read();
  return (*readable)->AcquireReliable();
}

statistics::ConnectionStatistics::Frozen
ClientImpl::MonitoredConnection::GetStatistics() const {
  return stats_.Get();
}

bool ClientImpl::MonitoredConnection::IsBroken() {
  auto readable = connection_.Read();
  return (*readable)->IsBroken();
}

ClientImpl::CopyableAtomic::CopyableAtomic() = default;

ClientImpl::CopyableAtomic::CopyableAtomic(const CopyableAtomic& other)
    : atomic{other.atomic.load()} {}

ClientImpl::CopyableAtomic& ClientImpl::CopyableAtomic::operator=(
    const CopyableAtomic& other) {
  if (this == &other) {
    return *this;
  }

  atomic = other.atomic.load();
  return *this;
}

engine::ev::ThreadControl& ClientImpl::GetNextEvThread() const {
  return owned_ev_pool_ == nullptr ? engine::current_task::GetEventThread()
                                   : owned_ev_pool_->NextThread();
}

std::size_t ClientImpl::CalculateConnectionsCountPerHost() const {
  const auto thread_count =
      owned_ev_pool_ == nullptr
          ? engine::current_task::GetTaskProcessor().EventThreadPool().GetSize()
          : owned_ev_pool_->GetSize();

  return thread_count * settings_.connections_per_thread;
}

ClientImpl::MonitoredConnection& ClientImpl::GetNextConnection() {
  const auto host_idx = host_idx_++ % settings_.endpoints.endpoints.size();
  const auto conn_idx =
      connections_per_host_ * host_idx +
      (host_conn_idx_[host_idx].atomic++ % connections_per_host_);
  return *connections_[conn_idx];
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
