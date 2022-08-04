#include "client_impl.hpp"

#include <engine/ev/thread_pool.hpp>
#include <engine/task/task_processor.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/wait_all_checked.hpp>
#include <userver/utils/assert.hpp>

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

std::shared_ptr<Connection> TryCreateConnectionPtr(
    clients::dns::Resolver& resolver, engine::ev::ThreadControl& thread,
    const ConnectionSettings& connection_settings, const EndpointInfo& endpoint,
    const AuthSettings& auth_settings,
    statistics::ConnectionStatistics& stats) {
  try {
    return CreateConnectionPtr(resolver, thread, connection_settings, endpoint,
                               auth_settings, stats);
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Failed to initialize a connection: " << ex.what()
                << "; will retry";
    return nullptr;
  }
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

  auto host_connection_it_begin = connections_.begin();
  for (const auto& endpoint : settings_.endpoints.endpoints) {
    auto host_connection_it_end =
        host_connection_it_begin + connections_per_host_;
    hosts_.emplace_back(endpoint, host_connection_it_begin,
                        host_connection_it_end);
    host_connection_it_begin = host_connection_it_end;
  }
}

ChannelPtr ClientImpl::GetUnreliable() { return GetNextConnection().Acquire(); }

ChannelPtr ClientImpl::GetReliable() {
  return GetNextConnection().AcquireReliable();
}

formats::json::Value ClientImpl::GetStatistics() const {
  formats::json::ValueBuilder builder{formats::json::Type::kObject};

  for (const auto& host : hosts_) {
    builder[host.GetHostName()] = host.GetStatistics();
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
      connection_{TryCreateConnectionPtr(resolver_, ev_thread_,
                                         connection_settings_, endpoint_,
                                         auth_settings_, stats_)} {
  monitor_.Start(
      "connection_monitor", {std::chrono::milliseconds{1000}}, [this] {
        if (IsBroken()) {
          LOG_WARNING() << "Connection is broken, trying to reconnect";
          try {
            connection_.Emplace(
                CreateConnectionPtr(resolver_, ev_thread_, connection_settings_,
                                    endpoint_, auth_settings_, stats_));
            stats_.AccountReconnectSuccess();
            LOG_INFO() << "Reconnected successfully";
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
  EnsureNotBroken();

  auto readable = connection_.Read();
  return (*readable)->Acquire();
}

ChannelPtr ClientImpl::MonitoredConnection::AcquireReliable() {
  EnsureNotBroken();

  auto readable = connection_.Read();
  return (*readable)->AcquireReliable();
}

statistics::ConnectionStatistics::Frozen
ClientImpl::MonitoredConnection::GetStatistics() const {
  return stats_.Get();
}

bool ClientImpl::MonitoredConnection::IsBroken() {
  auto readable = connection_.Read();
  return (*readable == nullptr) || (*readable)->IsBroken();
}

void ClientImpl::MonitoredConnection::EnsureNotBroken() {
  if (IsBroken()) {
    throw std::runtime_error{
        "Connection is broken and will be reestablished soon, retry later"};
  }
}

ClientImpl::Host::CopyableAtomic::CopyableAtomic() = default;

ClientImpl::Host::CopyableAtomic::CopyableAtomic(const CopyableAtomic& other)
    : atomic{other.atomic.load()} {}

ClientImpl::Host::CopyableAtomic& ClientImpl::Host::CopyableAtomic::operator=(
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

  const auto connections_per_host =
      thread_count * settings_.connections_per_thread;
  UINVARIANT(connections_per_host > 0, "Invalid settings");

  return connections_per_host;
}

ClientImpl::Host::Host(const EndpointInfo& endpoint,
                       ConnectionsStorage::iterator from,
                       ConnectionsStorage::iterator to)
    : host_{&endpoint.host} {
  connections_.reserve(std::distance(from, to));
  while (from != to) {
    connections_.push_back(from->get());
    ++from;
  }
}

ClientImpl::MonitoredConnection& ClientImpl::Host::GetConnection() {
  const auto start_idx = conn_idx_.atomic.load(std::memory_order_relaxed);
  for (size_t i = 0; i < connections_.size(); ++i) {
    const auto idx = (start_idx + i) % connections_.size();
    if (!connections_[idx]->IsBroken()) {
      conn_idx_.atomic.fetch_add(i, std::memory_order_relaxed);
      return *connections_[idx];
    }
  }

  throw std::runtime_error{"No available connection in host"};
}

statistics::ConnectionStatistics::Frozen ClientImpl::Host::GetStatistics()
    const {
  statistics::ConnectionStatistics::Frozen stats{};
  for (const auto* conn : connections_) {
    stats += conn->GetStatistics();
  }
  return stats;
}

const std::string& ClientImpl::Host::GetHostName() const { return *host_; }

ClientImpl::MonitoredConnection& ClientImpl::GetNextConnection() {
  const auto start_idx = host_idx_.load(std::memory_order_relaxed);
  for (size_t i = 0; i < hosts_.size(); ++i) {
    const auto idx = (start_idx + i) % hosts_.size();
    try {
      auto& connection = hosts_[idx].GetConnection();
      host_idx_.fetch_add(i, std::memory_order_relaxed);
      return connection;
    } catch (const std::exception&) {
    }
  }

  throw std::runtime_error{"No available connections in cluster"};
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
