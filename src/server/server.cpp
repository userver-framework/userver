#include "server.hpp"

#include <stdexcept>

#include <engine/task/task_processor.hpp>
#include <logging/log.hpp>

namespace {

const char* PER_LISTENER_DESC = "per-listener";
const char* PER_CONNECTION_DESC = "per-connection";

using Verbosity = components::MonitorVerbosity;

Json::Value SerializeAggregated(const server::net::Stats::AggregatedStat& agg,
                                Verbosity verbosity, const char* items_desc) {
  Json::Value json_agg(Json::objectValue);
  json_agg["total"] = Json::UInt64{agg.Total()};
  json_agg["max"] = Json::UInt64{agg.Max()};
  if (verbosity == Verbosity::kFull) {
    Json::Value json_items(Json::arrayValue);
    for (auto item : agg.Items()) {
      json_items[json_items.size()] = Json::UInt64{item};
    }
    json_agg[items_desc] = std::move(json_items);
  }
  return json_agg;
}

}  // namespace

namespace server {

Server::Server(ServerConfig config,
               const components::ComponentContext& component_context)
    : config_(std::move(config)), is_destroying_(false) {
  LOG_INFO() << "Creating server";

  engine::TaskProcessor* task_processor =
      component_context.GetTaskProcessor(config_.task_processor);
  if (!task_processor) {
    throw std::runtime_error("can't find task_processor '" +
                             config_.task_processor + "' for server");
  }

  request_handlers_ = CreateRequestHandlers(component_context);

  endpoint_info_ =
      std::make_shared<net::EndpointInfo>(config_.listener, *request_handlers_);

  auto& event_thread_pool = task_processor->EventThreadPool();
  size_t listener_shards = config_.listener.shards ? *config_.listener.shards
                                                   : event_thread_pool.size();
  auto event_thread_controls = event_thread_pool.NextThreads(listener_shards);
  for (auto* event_thread_control : event_thread_controls) {
    listeners_.emplace_back(endpoint_info_, *task_processor,
                            *event_thread_control);
  }

  LOG_INFO() << "Server is created";
}

Server::~Server() {
  {
    std::unique_lock<std::shared_timed_mutex> lock(stat_mutex_);
    is_destroying_ = true;
  }
  LOG_INFO() << "Stopping server";
  LOG_TRACE() << "Stopping listeners";
  listeners_.clear();
  LOG_TRACE() << "Stopped listeners";
  LOG_TRACE() << "Stopping request handlers";
  request_handlers_.reset();
  LOG_TRACE() << "Stopped request handlers";
  LOG_INFO() << "Stopped server";
}

const ServerConfig& Server::GetConfig() const { return config_; }

Json::Value Server::GetMonitorData(
    components::MonitorVerbosity verbosity) const {
  Json::Value json_data(Json::objectValue);

  auto server_stats = GetServerStats();
  {
    Json::Value json_conn_stats(Json::objectValue);
    json_conn_stats["active"] = SerializeAggregated(
        server_stats.active_connections, verbosity, PER_LISTENER_DESC);
    json_conn_stats["opened"] = SerializeAggregated(
        server_stats.total_opened_connections, verbosity, PER_LISTENER_DESC);
    json_conn_stats["closed"] = SerializeAggregated(
        server_stats.total_closed_connections, verbosity, PER_LISTENER_DESC);

    json_data["connections"] = std::move(json_conn_stats);
  }
  {
    Json::Value json_request_stats(Json::objectValue);
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

  return json_data;
}

bool Server::AddHandler(const handlers::HandlerBase& handler,
                        const components::ComponentContext& component_context) {
  return (handler.IsMonitor() ? request_handlers_->GetMonitorRequestHandler()
                              : request_handlers_->GetHttpRequestHandler())
      .AddHandler(handler, component_context);
}

void Server::Start() {
  LOG_INFO() << "Starting server";
  request_handlers_->GetMonitorRequestHandler().DisableAddHandler();
  request_handlers_->GetHttpRequestHandler().DisableAddHandler();
  for (auto& listener : listeners_) {
    listener.Start();
  }
  LOG_INFO() << "Server is started";
}

net::Stats Server::GetServerStats() const {
  net::Stats summary;

  std::shared_lock<std::shared_timed_mutex> lock(stat_mutex_);
  if (is_destroying_) return summary;
  for (const auto& listener : listeners_) {
    summary += listener.GetStats();
  }
  return summary;
}

std::unique_ptr<RequestHandlers> Server::CreateRequestHandlers(
    const components::ComponentContext& component_context) const {
  auto request_handlers = std::make_unique<RequestHandlers>();
  try {
    request_handlers->SetHttpRequestHandler(
        std::make_unique<http::HttpRequestHandler>(
            component_context, config_.logger_access,
            config_.logger_access_tskv, false));
  } catch (const std::exception& ex) {
    LOG_ERROR() << "can't create HttpRequestHandler: " << ex.what();
    throw;
  }
  try {
    request_handlers->SetMonitorRequestHandler(
        std::make_unique<http::HttpRequestHandler>(
            component_context, config_.logger_access,
            config_.logger_access_tskv, true));
  } catch (const std::exception& ex) {
    LOG_ERROR() << "can't create MonitorRequestHandler: " << ex.what();
    throw;
  }
  return request_handlers;
}

}  // namespace server
