#pragma once

#include <formats/json/value.hpp>

#include <components/component_context.hpp>
#include <components/manager.hpp>
#include <server/handlers/http_handler_base.hpp>

namespace engine {
class TaskProcessor;
}  // namespace engine

namespace server {

namespace net {
struct Stats;
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

  void Start();

  void Stop();

  RequestsView& GetRequestsView();

  void SetRpsRatelimit(std::optional<size_t> rps);

 private:
  std::unique_ptr<ServerImpl> pimpl;
};

}  // namespace server
