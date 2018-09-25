#include <server/server.hpp>

#include <stdexcept>

#include <logging/log.hpp>
#include <server/server_config.hpp>

#include <engine/task/task_processor.hpp>
#include <server/http/http_request_handler.hpp>
#include <server/net/endpoint_info.hpp>
#include <server/net/listener.hpp>
#include <server/net/stats.hpp>

namespace {

const char* PER_LISTENER_DESC = "per-listener";
const char* PER_CONNECTION_DESC = "per-connection";

using Verbosity = utils::statistics::Verbosity;

formats::json::ValueBuilder SerializeAggregated(
    const server::net::Stats::AggregatedStat& agg, Verbosity verbosity,
    const char* items_desc) {
  formats::json::ValueBuilder json_agg(formats::json::Type::kObject);
  json_agg["total"] = agg.Total();
  json_agg["max"] = agg.Max();
  if (verbosity == Verbosity::kFull) {
    formats::json::ValueBuilder json_items(formats::json::Type::kArray);
    for (auto item : agg.Items()) {
      json_items.PushBack(item);
    }
    json_agg[items_desc] = std::move(json_items);
  }
  return json_agg;
}

}  // namespace

namespace server {

class ServerImpl {
 public:
  ServerImpl(ServerConfig config,
             const components::ComponentContext& component_context);
  ~ServerImpl();

  net::Stats GetServerStats() const;

  const ServerConfig config_;

  struct PortInfo {
    std::unique_ptr<http::HttpRequestHandler> request_handler_;
    std::shared_ptr<net::EndpointInfo> endpoint_info_;
    std::vector<net::Listener> listeners_;

    void Start();

    void Stop();
  };

  static void InitPortInfo(
      PortInfo& info, const ServerConfig& config,
      const net::ListenerConfig& listener_config,
      const components::ComponentContext& component_context, bool is_monitor);

  PortInfo main_port_info_, monitor_port_info_;

  mutable std::shared_timed_mutex stat_mutex_;
  bool is_destroying_;
};

void ServerImpl::PortInfo::Stop() {
  LOG_TRACE() << "Stopping listeners";
  listeners_.clear();
  LOG_TRACE() << "Stopped listeners";

  LOG_TRACE() << "Stopping request handlers";
  request_handler_.reset();
  LOG_TRACE() << "Stopped request handlers";
}

void ServerImpl::PortInfo::Start() {
  request_handler_->DisableAddHandler();
  for (auto& listener : listeners_) {
    listener.Start();
  }
}

ServerImpl::ServerImpl(ServerConfig config,
                       const components::ComponentContext& component_context)
    : config_(std::move(config)), is_destroying_(false) {
  LOG_INFO() << "Creating server";

  InitPortInfo(main_port_info_, config, config_.listener, component_context,
               false);
  InitPortInfo(monitor_port_info_, config, config_.monitor_listener,
               component_context, true);

  LOG_INFO() << "Server is created";
}

void ServerImpl::InitPortInfo(
    PortInfo& info, const ServerConfig& config,
    const net::ListenerConfig& listener_config,
    const components::ComponentContext& component_context, bool is_monitor) {
  LOG_INFO() << "Creating listener" << (is_monitor ? " (monitor)" : "");

  engine::TaskProcessor* task_processor =
      component_context.GetTaskProcessor(listener_config.task_processor);
  if (!task_processor) {
    throw std::runtime_error("can't find task_processor '" +
                             listener_config.task_processor + "' for server");
  }

  info.request_handler_ = std::make_unique<http::HttpRequestHandler>(
      component_context, config.logger_access, config.logger_access_tskv,
      is_monitor);

  info.endpoint_info_ = std::make_shared<net::EndpointInfo>(
      listener_config, *info.request_handler_);

  auto& event_thread_pool = task_processor->EventThreadPool();
  size_t listener_shards = listener_config.shards ? *listener_config.shards
                                                  : event_thread_pool.size();
  auto event_thread_controls = event_thread_pool.NextThreads(listener_shards);
  for (auto* event_thread_control : event_thread_controls) {
    info.listeners_.emplace_back(info.endpoint_info_, *task_processor,
                                 *event_thread_control);
  }
}

ServerImpl::~ServerImpl() {
  {
    std::unique_lock<std::shared_timed_mutex> lock(stat_mutex_);
    is_destroying_ = true;
  }
  LOG_INFO() << "Stopping server";

  main_port_info_.Stop();
  monitor_port_info_.Stop();
  LOG_INFO() << "Stopped server";
}

Server::Server(ServerConfig config,
               const components::ComponentContext& component_context)
    : pimpl(
          std::make_unique<ServerImpl>(std::move(config), component_context)) {}

Server::~Server() = default;

const ServerConfig& Server::GetConfig() const { return pimpl->config_; }

formats::json::Value Server::GetMonitorData(
    utils::statistics::Verbosity verbosity) const {
  formats::json::ValueBuilder json_data(formats::json::Type::kObject);

  auto server_stats = pimpl->GetServerStats();
  {
    formats::json::ValueBuilder json_conn_stats(formats::json::Type::kObject);
    json_conn_stats["active"] = SerializeAggregated(
        server_stats.active_connections, verbosity, PER_LISTENER_DESC);
    json_conn_stats["opened"] = SerializeAggregated(
        server_stats.total_opened_connections, verbosity, PER_LISTENER_DESC);
    json_conn_stats["closed"] = SerializeAggregated(
        server_stats.total_closed_connections, verbosity, PER_LISTENER_DESC);

    json_data["connections"] = std::move(json_conn_stats);
  }
  {
    formats::json::ValueBuilder json_request_stats(
        formats::json::Type::kObject);
    json_request_stats["active"] = SerializeAggregated(
        server_stats.active_requests, verbosity, PER_CONNECTION_DESC);
    json_request_stats["parsing"] = SerializeAggregated(
        server_stats.parsing_requests, verbosity, PER_CONNECTION_DESC);
    json_request_stats["pending-response"] = SerializeAggregated(
        server_stats.pending_responses, verbosity, PER_CONNECTION_DESC);
    json_request_stats["conn-processed"] = SerializeAggregated(
        server_stats.conn_processed_requests, verbosity, PER_CONNECTION_DESC);
    json_request_stats["listener-processed"] = SerializeAggregated(
        server_stats.listener_processed_requests, verbosity, PER_LISTENER_DESC);

    json_data["requests"] = std::move(json_request_stats);
  }

  return json_data.ExtractValue();
}

bool Server::AddHandler(const handlers::HandlerBase& handler,
                        const components::ComponentContext& component_context) {
  return (handler.IsMonitor() ? pimpl->monitor_port_info_.request_handler_
                              : pimpl->main_port_info_.request_handler_)
      ->AddHandler(handler, component_context);
}

void Server::Start() {
  LOG_INFO() << "Starting server";
  pimpl->main_port_info_.Start();
  pimpl->monitor_port_info_.Start();
  LOG_INFO() << "Server is started";
}

net::Stats ServerImpl::GetServerStats() const {
  net::Stats summary;

  std::shared_lock<std::shared_timed_mutex> lock(stat_mutex_);
  if (is_destroying_) return summary;
  for (const auto& listener : main_port_info_.listeners_) {
    summary += listener.GetStats();
  }

  return summary;
}

}  // namespace server
