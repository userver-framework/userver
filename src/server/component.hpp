#pragma once

#include <memory>

#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <components/monitorable_component_base.hpp>

#include "server.hpp"

namespace components {

class Server : public MonitorableComponentBase {
 public:
  static constexpr const char* kName = "server";

  Server(const components::ComponentConfig& component_config,
         const components::ComponentContext& component_context);

  void OnAllComponentsLoaded() override;

  const server::Server& GetServer() const;

  bool AddHandler(const server::handlers::HandlerBase& handler,
                  const components::ComponentContext& component_context);
  Json::Value GetMonitorData(MonitorVerbosity verbosity) const override;

 private:
  std::unique_ptr<server::Server> server_;
};

}  // namespace components
