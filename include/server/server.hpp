#pragma once

#include <formats/json/value.hpp>

#include <components/component_context.hpp>
#include <components/manager.hpp>
#include <server/handlers/handler_base.hpp>

namespace server {

class ServerImpl;
class ServerConfig;

class Server {
 public:
  Server(ServerConfig config,
         const components::ComponentContext& component_context);
  ~Server();

  const ServerConfig& GetConfig() const;
  formats::json::Value GetMonitorData(
      components::MonitorVerbosity verbosity) const;
  bool AddHandler(const handlers::HandlerBase& handler,
                  const components::ComponentContext& component_context);

  void Start();

 private:
  std::unique_ptr<ServerImpl> pimpl;
};

}  // namespace server
