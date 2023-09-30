#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_component_list.hpp>
#include <userver/fs/read.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/telegram/bot/client/client.hpp>
#include <userver/telegram/bot/components/client.hpp>
#include <userver/telegram/bot/components/long_poller.hpp>

#include <codecvt>
#include <locale>

namespace samples::telegram_bot {

class GreetUser final : public telegram::bot::TelegramBotLongPoller {
 public:
  static constexpr std::string_view kName = "handler-greet-user";

  GreetUser(const components::ComponentConfig& config,
            const components::ComponentContext& context);
  
  void HandleUpdate(telegram::bot::Update update,
                    telegram::bot::ClientPtr client) override;
  
  static yaml_config::Schema GetStaticConfigSchema();

 private:
  std::int64_t SendGreeting(
      telegram::bot::ClientPtr client,
      std::int64_t chat_id,
      const std::optional<std::string>& username) const;

  static std::string GreetingCaption(
      const std::optional<std::string>& username);

  static std::vector<telegram::bot::MessageEntity> GreetingMessageEntities(
      std::string_view caption);

  telegram::bot::SendPhotoMethod::Parameters::Photo GreetingPhoto() const;

  static std::shared_ptr<std::string> ReadPhoto(
      const components::ComponentConfig& config,
      const components::ComponentContext& context);

  telegram::bot::RequestOptions request_options_;
  mutable rcu::Variable<std::string> photo_file_id_;
  const std::shared_ptr<std::string> photo_;
};

}  // namespace samples::telegram_bot

namespace samples::telegram_bot {

GreetUser::GreetUser(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
    : telegram::bot::TelegramBotLongPoller (config, context),
      request_options_{std::chrono::seconds{5}},
      photo_(ReadPhoto(config, context)) {}

void GreetUser::HandleUpdate(telegram::bot::Update update,
                             telegram::bot::ClientPtr client) {
  if (!update.message) {
    return;
  }
  try {
    const std::int64_t chat_id = update.message->chat->id;
    const auto& username = update.message->chat->first_name;
    SendGreeting(client, chat_id, username);
  } catch (std::exception& ex) {
    LOG_ERROR() << "Error send greeting: " << ex.what();
  }
}

yaml_config::Schema GreetUser::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<telegram::bot::TelegramBotLongPoller>(R"(
type: object
description: The component for sending a greeting to a user on behalf of a bot
additionalProperties: false
properties:
    fs-task-processor:
        type: string
        description: fs-task-processor
    greeting-photo-path:
        type: string
        description: The path to the greeting picture
)");
  }

std::int64_t GreetUser::SendGreeting(
    telegram::bot::ClientPtr client,
    std::int64_t chat_id,
    const std::optional<std::string>& username) const {
  auto upload_action = telegram::bot::SendChatActionMethod::Parameters::Action::kUploadPhoto;
  auto send_action_request = client->SendChatAction({chat_id, upload_action},
                                                      request_options_);
  send_action_request.Perform();

  telegram::bot::SendPhotoMethod::Parameters parameters{chat_id,
                                                        GreetingPhoto()};
  parameters.caption = GreetingCaption(username);
  parameters.caption_entities = GreetingMessageEntities(
     parameters.caption.value());

  auto send_photo_request = client->SendPhoto(parameters, request_options_);
  auto result = send_photo_request.Perform();
  if (!result.photo->empty()) {
    auto file_id = photo_file_id_.StartWrite();
    *file_id = std::move(result.photo->back().file_id);
    file_id.Commit();
  }
  return result.message_id;
}

std::string GreetUser::GreetingCaption(
    const std::optional<std::string>& username) {
  if (username) {
    return fmt::format("Hello, {}! This is a test telegram bot written on "
                       "userver.",
                       username.value());
  } else {
    return "Hello! This is a test telegram bot written on userver.";
  }
}

std::vector<telegram::bot::MessageEntity> GreetUser::GreetingMessageEntities(
      std::string_view caption) {
  std::vector<telegram::bot::MessageEntity> result;
  telegram::bot::MessageEntity& linkEntity = result.emplace_back();

  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
  std::wstring wcaption = convert.from_bytes(caption.data());

  linkEntity.offset = wcaption.size() - 8;
  linkEntity.length = 7;
  linkEntity.type = telegram::bot::MessageEntity::Type::kTextLink;
  linkEntity.url = "https://userver.tech/index.html";
  return result;
}

telegram::bot::SendPhotoMethod::Parameters::Photo GreetUser::GreetingPhoto() const {
  {
    auto file_id = photo_file_id_.Read();
    if (!file_id->empty()) {
      return *file_id;
    }
  }
  return telegram::bot::InputFile{photo_,
                                  "/userver_greeting_photo.jpg",
                                  "image/jpeg"};
}

std::shared_ptr<std::string> GreetUser::ReadPhoto(
    const components::ComponentConfig& config,
    const components::ComponentContext& context) {
  const auto fs_tp_name = config["fs-task-processor"].As<std::string>();
  const auto greeting_photo_path = config["greeting-photo-path"]
                                      .As<std::string>();
  auto& fs_task_processor = context.GetTaskProcessor(fs_tp_name);
  return std::make_shared<std::string>(
      fs::ReadFileContents(fs_task_processor, greeting_photo_path));
}

}  // namespace samples::telegram_bot

int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalComponentList()
          .Append<telegram::bot::TelegramBotClient>()
          .Append<samples::telegram_bot::GreetUser>()
          .Append<clients::dns::Component>()
          .Append<components::HttpClient>();
  return utils::DaemonMain(argc, argv, component_list);
}
