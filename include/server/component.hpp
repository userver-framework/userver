#pragma once

#include <memory>

#include <components/component_base.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <server/server.hpp>

namespace engine {
class TaskProcessor;
}  // namespace engine

namespace components {

class StatisticsStorage;

class Server : public ComponentBase {
 public:
  static constexpr const char* kName = "server";

  Server(const components::ComponentConfig& component_config,
         const components::ComponentContext& component_context);

  ~Server();

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
  StatisticsStorage* statistics_storage_;
  utils::statistics::Entry statistics_holder_;
};

}  // namespace components
