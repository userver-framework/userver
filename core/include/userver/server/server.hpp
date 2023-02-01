#pragma once

#include <userver/formats/json/value.hpp>

#include <userver/components/component_context.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/server/handlers/fallback_handlers.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utils/statistics/fwd.hpp>

USERVER_NAMESPACE_BEGIN

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

  void WriteMonitorData(utils::statistics::Writer& writer) const;

  void WriteTotalHandlerStatistics(utils::statistics::Writer& writer) const;

  net::Stats GetServerStats() const;

  void AddHandler(const handlers::HttpHandlerBase& handler,
                  engine::TaskProcessor& task_processor);

  size_t GetRegisteredHandlersCount() const;

  const http::HttpRequestHandler& GetHttpRequestHandler(
      bool is_monitor = false) const;

  void Start();

  void Stop();

  RequestsView& GetRequestsView();

  void SetRpsRatelimit(std::optional<size_t> rps);

  void SetRpsRatelimitStatusCode(http::HttpStatus status_code);

 private:
  std::unique_ptr<ServerImpl> pimpl;
};

}  // namespace server

USERVER_NAMESPACE_END
