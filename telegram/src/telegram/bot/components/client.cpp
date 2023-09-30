#include <userver/telegram/bot/components/client.hpp>

#include <telegram/bot/client/client_impl.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
constexpr std::string_view kDefaultTelegramFqdn = "api.telegram.org";
}  // namespace

namespace telegram::bot {

TelegramBotClient::TelegramBotClient(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : LoggableComponentBase(config, context),
      client_(std::make_shared<ClientImpl>(
        context.FindComponent<components::HttpClient>().GetHttpClient(),
        config["bot-token"].As<std::string>(),
        config["telegram-fqdn"].As<std::string>(kDefaultTelegramFqdn))) {}

ClientPtr TelegramBotClient::GetClient() {
    return client_;
}

yaml_config::Schema TelegramBotClient::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: Telegram Bot Client component
additionalProperties: false
properties:
    bot-token:
        type: string
        description: telegram bot authentication token
    telegram-fqdn:
        type: string
        description: telegram fqdn
        defaultDescription: "api.telegram.org"
)");
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
