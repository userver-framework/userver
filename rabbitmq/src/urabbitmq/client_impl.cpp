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
    size_t max_channels, const EndpointInfo& endpoint,
    const AuthSettings& auth_settings) {
  return std::make_shared<Connection>(resolver, thread, endpoint, auth_settings,
                                      max_channels);
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
  UINVARIANT(!settings.endpoints.empty(), "empty set of hosts");
  connections_.resize(connections_per_host_ * settings_.endpoints.size());
  host_conn_idx_.assign(settings_.endpoints.size(), CopyableAtomic{});

  std::vector<engine::TaskWithResult<void>> init_tasks;
  init_tasks.reserve(connections_.size());

  for (size_t i = 0; i < connections_.size(); ++i) {
    init_tasks.emplace_back(engine::AsyncNoSpan([this, &resolver, i] {
      connections_[i] = std::make_unique<MonitoredConnection>(
          resolver, GetNextEvThread(), settings_.channels_per_connection,
          settings_.endpoints[i / connections_per_host_], settings_.auth);
    }));
  }
  engine::WaitAllChecked(init_tasks);
}

ChannelPtr ClientImpl::GetUnreliable() { return GetNextConnection().Acquire(); }

ChannelPtr ClientImpl::GetReliable() {
  return GetNextConnection().AcquireReliable();
}

ClientImpl::MonitoredConnection::MonitoredConnection(
    clients::dns::Resolver& resolver, engine::ev::ThreadControl& thread,
    size_t max_channels, const EndpointInfo& endpoint,
    const AuthSettings& auth_settings)
    : resolver_{resolver},
      ev_thread_{thread},
      endpoint_{endpoint},
      auth_settings_{auth_settings},
      max_channels_{max_channels},
      connection_{CreateConnectionPtr(resolver_, ev_thread_, max_channels_,
                                      endpoint_, auth_settings_)} {
  monitor_.Start(
      "connection_monitor", {std::chrono::milliseconds{1000}}, [this] {
        if (IsBroken()) {
          try {
            connection_.Emplace(CreateConnectionPtr(resolver_, ev_thread_,
                                                    max_channels_, endpoint_,
                                                    auth_settings_));
          } catch (const std::exception& ex) {
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

bool ClientImpl::MonitoredConnection::IsBroken() {
  auto readable = connection_.Read();
  return (*readable)->IsBroken();
}

ClientImpl::CopyableAtomic::CopyableAtomic() = default;

ClientImpl::CopyableAtomic::CopyableAtomic(const CopyableAtomic& other)
    : atomic{other.atomic.load()} {}

ClientImpl::CopyableAtomic& ClientImpl::CopyableAtomic::operator=(
    const CopyableAtomic& other) {
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
  const auto host_idx = host_idx_++ % settings_.endpoints.size();
  const auto conn_idx =
      connections_per_host_ * host_idx +
      (host_conn_idx_[host_idx].atomic++ % connections_per_host_);
  return *connections_[conn_idx];
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
