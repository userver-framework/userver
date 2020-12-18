#pragma once

#include <formats/json/value.hpp>

#include <components/component_context.hpp>
#include <components/manager.hpp>
#include <engine/task/task_processor_fwd.hpp>
#include <server/handlers/fallback_handlers.hpp>
#include <server/handlers/http_handler_base.hpp>

namespace server {

namespace net {
struct Stats;
}

namespace http {
class HttpRequestHandler;
}

class RequestsView;

class ServerImpl;
struct ServerConfig;

class Server final {
 public:
  Server(ServerConfig config,
         const components::ComponentContext& component_context);
  ~Server();

  const ServerConfig& GetConfig() const;

  formats::json::Value GetMonitorData(
      utils::statistics::StatisticsRequest) const;

  net::Stats GetServerStats() const;

  void AddHandler(const handlers::HttpHandlerBase& handler,
                  engine::TaskProcessor& task_processor);

  const http::HttpRequestHandler& GetHttpRequestHandler(
      bool is_monitor = false) const;

  void Start();

  void Stop();

  RequestsView& GetRequestsView();

  void SetRpsRatelimit(std::optional<size_t> rps);

 private:
  std::unique_ptr<ServerImpl> pimpl;
};

}  // namespace server
