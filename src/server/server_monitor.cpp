#include "server_monitor.hpp"

#include <json/value.h>
#include <json/writer.h>

#include <server/http/http_request.hpp>

#include "server_impl.hpp"

namespace server {

ServerMonitor::ServerMonitor(const ServerImpl& server_impl)
    : server_impl_(server_impl) {}

std::string ServerMonitor::GetJsonData(
    const request::RequestBase& request) const {
  const auto& http_request = dynamic_cast<const http::HttpRequest&>(request);
  const bool is_full = http_request.GetArg("full") == "1";

  Json::Value json_data(Json::objectValue);
  if (is_full) {
    const auto& config = server_impl_.GetConfig();
    Json::Value json_task_processors(Json::arrayValue);
    for (const auto& task_processor : config.task_processors) {
      Json::Value json_task_processor(Json::objectValue);
      json_task_processor["name"] = task_processor.name;
      json_task_processor["worker_threads"] =
          Json::UInt64{task_processor.worker_threads};
      json_task_processors[json_task_processors.size()] =
          std::move(json_task_processor);
    }
    json_data["task_processors"] = std::move(json_task_processors);
  }

  auto network_stats = server_impl_.GetNetworkStats();
  json_data["processed_request_count"] =
      Json::UInt64{network_stats.total_processed_requests};
  json_data["total_connections"] =
      Json::UInt64{network_stats.total_connections};
  json_data["max_connections"] =
      Json::UInt64{network_stats.max_listener_connections};
  json_data["request_count"] =
      Json::UInt64{network_stats.total_active_requests};
  json_data["parsing_requests"] =
      Json::UInt64{network_stats.total_parsing_requests};

  Json::Value json_connections(Json::arrayValue);
  for (auto conn_count : network_stats.listener_connections) {
    json_connections[json_connections.size()] = Json::UInt64{conn_count};
  }
  json_data["connections"] = std::move(json_connections);

  if (is_full) {
    Json::Value json_request_queues(Json::arrayValue);
    for (auto queue_size : network_stats.conn_active_requests) {
      json_request_queues[json_request_queues.size()] =
          Json::UInt64{queue_size};
    }
    json_data["request_queue_sizes"] = std::move(json_request_queues);
  }

  size_t pending_responses_total = 0;
  Json::Value json_waiting_responses(Json::arrayValue);
  for (auto queue_size : network_stats.conn_pending_responses) {
    pending_responses_total += queue_size;
    if (is_full) {
      json_waiting_responses[json_waiting_responses.size()] =
          Json::UInt64{queue_size};
    }
  }
  json_data["waiting_responses_total"] = Json::UInt64{pending_responses_total};
  if (is_full) {
    json_data["waiting_responses"] = std::move(json_waiting_responses);
  }

  auto coro_stats = server_impl_.GetCoroutineStats();
  Json::Value json_coro_pool(Json::objectValue);
  json_coro_pool["active_coros"] = Json::UInt64{coro_stats.active_coroutines};
  json_coro_pool["total_coros"] = Json::UInt64{coro_stats.total_coroutines};
  json_data["coro_pool"] = std::move(json_coro_pool);

  return Json::FastWriter().write(json_data);
}

}  // namespace server
