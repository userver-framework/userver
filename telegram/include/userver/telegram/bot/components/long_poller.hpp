#pragma once

#include <userver/telegram/bot/client/client_fwd.hpp>
#include <userver/telegram/bot/types/update.hpp>

#include <userver/components/component.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/concurrent/background_task_storage.hpp>
#include <userver/utils/periodic_task.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

class TelegramBotLongPoller : public components::LoggableComponentBase {
public:
  static constexpr std::string_view kName = "telegram-bot-long-poller";

  TelegramBotLongPoller(const components::ComponentConfig& config,
                        const components::ComponentContext& context);

  virtual void HandleUpdate(Update update, ClientPtr client) = 0;

  void OnAllComponentsLoaded() override;

  void OnAllComponentsAreStopping() override;

  static yaml_config::Schema GetStaticConfigSchema();

private:
  void FetchAndHandleUpdates();

  std::vector<Update> FetchUpdates();

  ClientPtr client_;

  std::int64_t offset_ = 0;

  const std::chrono::milliseconds pulling_frequency_;
  const std::chrono::milliseconds polling_timeout_;

  utils::PeriodicTask periodic_;

  concurrent::BackgroundTaskStorage task_storage_;
};

}  // namespace telegram::bot

USERVER_NAMESPACE_END
