#pragma once

#include <memory>

#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <components/loggable_component_base.hpp>
#include <engine/task/task_processor_fwd.hpp>
#include <server/server.hpp>

namespace components {

class StatisticsStorage;

/// @ingroup userver_components
class Server final : public LoggableComponentBase {
 public:
  static constexpr const char* kName = "server";

  Server(const components::ComponentConfig& component_config,
         const components::ComponentContext& component_context);

  ~Server() override;

  void OnAllComponentsLoaded() override;

  void OnAllComponentsAreStopping() override;

  const server::Server& GetServer() const;

  server::Server& GetServer();

  void AddHandler(const server::handlers::HttpHandlerBase& handler,
                  engine::TaskProcessor& task_processor);

 private:
  formats::json::Value ExtendStatistics(
      const utils::statistics::StatisticsRequest& /*request*/);

 private:
  std::unique_ptr<server::Server> server_;
  StatisticsStorage& statistics_storage_;
  utils::statistics::Entry statistics_holder_;
};

}  // namespace components
