#pragma once

#include <json/value.h>

#include <yandex/taxi/userver/components/component_context.hpp>
#include <yandex/taxi/userver/components/manager.hpp>
#include <yandex/taxi/userver/server/handlers/handler_base.hpp>

namespace server {

class ServerImpl;
class ServerConfig;

class Server {
 public:
  Server(ServerConfig config,
         const components::ComponentContext& component_context);
  ~Server();

  const ServerConfig& GetConfig() const;
  Json::Value GetMonitorData(components::MonitorVerbosity verbosity) const;
  bool AddHandler(const handlers::HandlerBase& handler,
                  const components::ComponentContext& component_context);

  void Start();

 private:
  std::unique_ptr<ServerImpl> pimpl;
};


}  // namespace server
