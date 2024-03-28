#pragma once

#include <userver/telegram/bot/client/client_fwd.hpp>

#include <userver/components/component.hpp>
#include <userver/components/loggable_component_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

class TelegramBotClient : public components::LoggableComponentBase {
public:
  static constexpr std::string_view kName = "telegram-bot-client";

  TelegramBotClient(const components::ComponentConfig& config,
                    const components::ComponentContext& context);

  static yaml_config::Schema GetStaticConfigSchema();

  ClientPtr GetClient();

private:
  ClientPtr client_;
};

}  // namespace telegram::bot

USERVER_NAMESPACE_END
