#pragma once

#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>

#include <json/value.h>

#include <components/component_context.hpp>
#include <components/manager.hpp>
#include <server/net/endpoint_info.hpp>
#include <server/net/listener.hpp>
#include <server/net/stats.hpp>
#include <server/request_handlers/request_handlers.hpp>

#include "server_config.hpp"

namespace server {

class Server {
 public:
  Server(ServerConfig config,
         const components::ComponentContext& component_context);
  ~Server();

  const ServerConfig& GetConfig() const;
  Json::Value GetMonitorData(components::MonitorVerbosity verbosity) const;

 private:
  net::Stats GetServerStats() const;
  std::unique_ptr<RequestHandlers> CreateRequestHandlers(
      const components::ComponentContext& component_context) const;

  const ServerConfig config_;

  std::unique_ptr<RequestHandlers> request_handlers_;
  std::shared_ptr<net::EndpointInfo> endpoint_info_;

  mutable std::shared_timed_mutex stat_mutex_;
  std::vector<net::Listener> listeners_;
  bool is_destroying_;
};

}  // namespace server
