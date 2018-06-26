#include "server_monitor.hpp"

#include <json/value.h>
#include <json/writer.h>

#include <server/http/http_request.hpp>
#include <server/net/stats.hpp>

#include "server_impl.hpp"

namespace {

const char* PER_LISTENER_DESC = "per-listener";
const char* PER_CONNECTION_DESC = "per-connection";

enum class Verbosity { Terse, Full };

Json::Value SerializeAggregated(const server::net::Stats::AggregatedStat& agg,
                                Verbosity verbosity, const char* items_desc) {
  Json::Value json_agg(Json::objectValue);
  json_agg["total"] = Json::UInt64{agg.Total()};
  json_agg["max"] = Json::UInt64{agg.Max()};
  if (verbosity == Verbosity::Full) {
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

ServerMonitor::ServerMonitor(const ServerImpl& server_impl)
    : server_impl_(server_impl) {}

std::string ServerMonitor::GetJsonData(const http::HttpRequest& request) const {
  const auto verbosity =
      request.GetArg("full") == "1" ? Verbosity::Full : Verbosity::Terse;

  Json::Value json_data(Json::objectValue);
  if (verbosity == Verbosity::Full) {
    const auto& config = server_impl_.GetConfig();
    Json::Value json_task_processors(Json::arrayValue);
    for (const auto& task_processor : config.task_processors) {
      Json::Value json_task_processor(Json::objectValue);
      json_task_processor["name"] = task_processor.name;
      json_task_processor["worker-threads"] =
          Json::UInt64{task_processor.worker_threads};
      json_task_processors[json_task_processors.size()] =
          std::move(json_task_processor);
    }
    json_data["task-processors"] = std::move(json_task_processors);
  }

  auto network_stats = server_impl_.GetNetworkStats();
  {
    Json::Value json_conn_stats(Json::objectValue);
    json_conn_stats["active"] = SerializeAggregated(
        network_stats.active_connections, verbosity, PER_LISTENER_DESC);
    json_conn_stats["opened"] = SerializeAggregated(
        network_stats.total_opened_connections, verbosity, PER_LISTENER_DESC);
    json_conn_stats["closed"] = SerializeAggregated(
        network_stats.total_closed_connections, verbosity, PER_LISTENER_DESC);

    json_data["connections"] = std::move(json_conn_stats);
  }
  {
    Json::Value json_request_stats(Json::objectValue);
    json_request_stats["active"] = SerializeAggregated(
        network_stats.active_requests, verbosity, PER_CONNECTION_DESC);
    json_request_stats["parsing"] = SerializeAggregated(
        network_stats.parsing_requests, verbosity, PER_CONNECTION_DESC);
    json_request_stats["pending-response"] = SerializeAggregated(
        network_stats.pending_responses, verbosity, PER_CONNECTION_DESC);
    json_request_stats["conn-processed"] = SerializeAggregated(
        network_stats.conn_processed_requests, verbosity, PER_CONNECTION_DESC);
    json_request_stats["listener-processed"] =
        SerializeAggregated(network_stats.listener_processed_requests,
                            verbosity, PER_LISTENER_DESC);

    json_data["requests"] = std::move(json_request_stats);
  }

  auto coro_stats = server_impl_.GetCoroutineStats();
  {
    Json::Value json_coro_pool(Json::objectValue);

    Json::Value json_coro_stats(Json::objectValue);
    json_coro_stats["active"] = Json::UInt64{coro_stats.active_coroutines};
    json_coro_stats["total"] = Json::UInt64{coro_stats.total_coroutines};
    json_coro_pool["coroutines"] = std::move(json_coro_stats);

    json_data["coro-pool"] = std::move(json_coro_pool);
  }

  return Json::FastWriter().write(json_data);
}

}  // namespace server
