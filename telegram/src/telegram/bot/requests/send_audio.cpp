#include <userver/telegram/bot/requests/send_audio.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>
#include <telegram/bot/formats/value_builder.hpp>
#include <telegram/bot/requests/request_data.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize_duration.hpp>
#include <userver/formats/json/serialize_variant.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/parse/common.hpp>
#include <userver/formats/parse/variant.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
SendAudioMethod::Parameters Parse(
    const Value& data,
    formats::parse::To<SendAudioMethod::Parameters>) {
  SendAudioMethod::Parameters parameters{
    data["chat_id"].template As<ChatId>(),
    data["audio"].template As<SendAudioMethod::Parameters::Audio>()
  };
  parameters.message_thread_id = data["message_thread_id"].template As<std::optional<std::int64_t>>();
  parameters.caption = data["caption"].template As<std::optional<std::string>>();
  parameters.parse_mode = data["parse_mode"].template As<std::optional<std::string>>();
  parameters.caption_entities = data["caption_entities"].template As<std::optional<std::vector<MessageEntity>>>();
  parameters.duration = data["duration"].template As<std::optional<std::chrono::seconds>>();
  parameters.performer = data["performer"].template As<std::optional<std::string>>();
  parameters.title = data["title"].template As<std::optional<std::string>>();
  parameters.thumbnail = data["thumbnail"].template As<std::unique_ptr<InputFile>>();
  parameters.disable_notification = data["disable_notification"].template As<std::optional<bool>>();
  parameters.protect_content = data["protect_content"].template As<std::optional<bool>>();
  parameters.reply_to_message_id = data["reply_to_message_id"].template As<std::optional<std::int64_t>>();
  parameters.allow_sending_without_reply = data["allow_sending_without_reply"].template As<std::optional<bool>>();
  parameters.reply_markup = data["reply_markup"].template As<std::optional<ReplyMarkup>>();
  return parameters;
}

template <class Value>
Value Serialize(const SendAudioMethod::Parameters& parameters,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["chat_id"] = parameters.chat_id;
  SetIfNotNull(builder, "message_thread_id", parameters.message_thread_id);
  builder["audio"] = parameters.audio;
  SetIfNotNull(builder, "caption", parameters.caption);
  SetIfNotNull(builder, "parse_mode", parameters.parse_mode);
  SetIfNotNull(builder, "caption_entities", parameters.caption_entities);
  SetIfNotNull(builder, "duration", parameters.duration);
  SetIfNotNull(builder, "performer", parameters.performer);
  SetIfNotNull(builder, "title", parameters.title);
  SetIfNotNull(builder, "thumbnail", parameters.thumbnail);
  SetIfNotNull(builder, "disable_notification", parameters.disable_notification);
  SetIfNotNull(builder, "protect_content", parameters.protect_content);
  SetIfNotNull(builder, "reply_to_message_id", parameters.reply_to_message_id);
  SetIfNotNull(builder, "allow_sending_without_reply", parameters.allow_sending_without_reply);
  SetIfNotNull(builder, "reply_markup", parameters.reply_markup);
  return builder.ExtractValue();
}

}  // namespace impl

SendAudioMethod::Parameters::Parameters(
    ChatId _chat_id,
    SendAudioMethod::Parameters::Audio _audio)
  : chat_id(std::move(_chat_id))
  , audio(std::move(_audio)) {}

void SendAudioMethod::FillRequestData(clients::http::Request& request,
                                      const Parameters& parameters) {
  if (!std::get_if<InputFile>(&parameters.audio) && !parameters.thumbnail) {
    FillRequestDataAsJson<SendAudioMethod>(request, parameters);
    return;
  }

  clients::http::Form form;
  FillFormSection(form, "chat_id", parameters.chat_id);
  FillFormSection(form, "message_thread_id", parameters.message_thread_id);
  FillFormSection(form, "audio", parameters.audio);
  FillFormSection(form, "caption", parameters.caption);
  FillFormSection(form, "parse_mode", parameters.parse_mode);
  FillFormSection(form, "caption_entities", parameters.caption_entities);
  FillFormSection(form, "duration", parameters.duration);
  FillFormSection(form, "performer", parameters.performer);
  FillFormSection(form, "title", parameters.title);
  FillFormSection(form, "thumbnail", parameters.thumbnail);
  FillFormSection(form, "disable_notification", parameters.disable_notification);
  FillFormSection(form, "protect_content", parameters.protect_content);
  FillFormSection(form, "reply_to_message_id", parameters.reply_to_message_id);
  FillFormSection(form, "allow_sending_without_reply", parameters.allow_sending_without_reply);
  FillFormSection(form, "reply_markup", parameters.reply_markup);
  request.form(form);
}

SendAudioMethod::Reply SendAudioMethod::ParseResponseData(
    clients::http::Response& response) {
  return ParseResponseDataFromJson<SendAudioMethod>(response);
}

SendAudioMethod::Parameters Parse(
    const formats::json::Value& json,
    formats::parse::To<SendAudioMethod::Parameters> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(
    const SendAudioMethod::Parameters& parameters,
    formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(parameters, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
