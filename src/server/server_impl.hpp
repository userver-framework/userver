#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <components/component_base.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <engine/coro/pool_stats.hpp>
#include <engine/ev/thread_pool.hpp>
#include <server/net/endpoint_info.hpp>
#include <server/net/listener.hpp>
#include <server/net/stats.hpp>
#include <server/request_handling/request_handler.hpp>

#include "server_config.hpp"

namespace server {

class ComponentList;

class ServerImpl {
 public:
  ServerImpl(ServerConfig config, const ComponentList& component_list);
  ~ServerImpl();

  const ServerConfig& GetConfig() const;
  net::Stats GetNetworkStats() const;
  engine::coro::PoolStats GetCoroutineStats() const;

  template <typename Component>
  std::enable_if_t<std::is_base_of<components::ComponentBase, Component>::value>
  AddComponent(const components::ComponentConfigMap& config_map,
               const std::string& name) {
    AddComponentImpl(config_map, name,
                     [](const components::ComponentConfig& config,
                        const components::ComponentContext& context) {
                       return std::make_unique<Component>(config, context);
                     });
  }

  void OnLogRotate();

 private:
  void AddComponentImpl(
      const components::ComponentConfigMap& config_map, const std::string& name,
      std::function<std::unique_ptr<components::ComponentBase>(
          const components::ComponentConfig&,
          const components::ComponentContext&)>
          factory);

  const ServerConfig config_;

  std::unique_ptr<engine::TaskProcessor::CoroPool> coro_pool_;
  std::unordered_map<std::string, engine::ev::ThreadPool> event_thread_pools_;
  std::unique_ptr<components::ComponentContext> component_context_;
  engine::TaskProcessor* default_task_processor_;
  std::unique_ptr<request_handling::RequestHandler> request_handler_;
  std::shared_ptr<net::EndpointInfo> endpoint_info_;
  std::vector<net::Listener> listeners_;
};

}  // namespace server
