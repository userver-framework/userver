#include <userver/telegram/bot/components/long_poller.hpp>

#include <userver/telegram/bot/client/client.hpp>
#include <userver/telegram/bot/components/client.hpp>

#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
constexpr std::chrono::milliseconds kDefaultPollingFrequency =
    std::chrono::seconds{1};
constexpr std::chrono::milliseconds kDefaultPollingTimeout =
    std::chrono::minutes{10};
}  // namespace

namespace telegram::bot {

TelegramBotLongPoller::TelegramBotLongPoller(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::LoggableComponentBase(config, context)
    , client_(context.FindComponent<TelegramBotClient>()
                .GetClient())
    , polling_frequency_(
          config["polling-frequency"].As<std::chrono::milliseconds>(
            kDefaultPollingFrequency))
    , polling_timeout_(
        config["polling-timeout"].As<std::chrono::milliseconds>(
            kDefaultPollingTimeout)) {}

void TelegramBotLongPoller::OnAllComponentsLoaded() {
  periodic_.Start(
    fmt::format("{}/long_polling_periodic_task", kName),
    {polling_frequency_},
    [this] { FetchAndHandleUpdates(); }
  );
}

void TelegramBotLongPoller::OnAllComponentsAreStopping() {
  periodic_.Stop();
  task_storage_.CancelAndWait();
}

yaml_config::Schema TelegramBotLongPoller::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: Component that provides updates via long polling
additionalProperties: false
properties:
    polling-frequency:
        type: string
        description: The frequency with which updates from telegram will
                     be polled
        defaultDescription: 1s
    polling-timeout:
        type: string
        description: Timeout in seconds for long polling
        defaultDescription: 60s
)");
}

void TelegramBotLongPoller::FetchAndHandleUpdates() {
  std::vector<Update> updates;
  try {
    updates = FetchUpdates();
  } catch (std::exception& ex) {
    LOG_ERROR() << "Error receiving updates: " << ex.what();
    return;
  }
  for (Update& update : updates) {
    std::int64_t update_id = update.update_id;
    std::string task_name = fmt::format("{}/handle_update/{}",
                                        kName, update_id);
    auto handle_update = [this] (Update update) {
      HandleUpdate(std::move(update), client_);
    };
    task_storage_.AsyncDetach(std::move(task_name),
                              handle_update, std::move(update));
    offset_ = std::max(offset_, update_id + 1);
  }
}

std::vector<Update> TelegramBotLongPoller::FetchUpdates() {
  GetUpdatesMethod::Parameters parameters;
  parameters.offset = offset_;
  parameters.limit = 100;
  parameters.timeout =
      std::chrono::duration_cast<std::chrono::seconds>(polling_timeout_);
  RequestOptions request_options;
  request_options.timeout = polling_timeout_ + std::chrono::seconds{1};
  request_options.retries = 1;
  return client_->GetUpdates(parameters, request_options).Perform();
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
