#include <userver/telegram/bot/requests/send_video.hpp>

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
SendVideoMethod::Parameters Parse(
    const Value& data,
    formats::parse::To<SendVideoMethod::Parameters>) {
  SendVideoMethod::Parameters parameters{
    data["chat_id"].template As<ChatId>(),
    data["video"].template As<SendVideoMethod::Parameters::Video>()
  };
  parameters.message_thread_id = data["message_thread_id"].template As<std::optional<std::int64_t>>();
  parameters.duration = data["duration"].template As<std::optional<std::chrono::seconds>>();
  parameters.width = data["width"].template As<std::optional<std::int64_t>>();
  parameters.height = data["height"].template As<std::optional<std::int64_t>>();
  parameters.thumbnail = data["thumbnail"].template As<std::unique_ptr<InputFile>>();
  parameters.caption = data["caption"].template As<std::optional<std::string>>();
  parameters.parse_mode = data["parse_mode"].template As<std::optional<std::string>>();
  parameters.caption_entities = data["caption_entities"].template As<std::optional<std::vector<MessageEntity>>>();
  parameters.has_spoiler = data["has_spoiler"].template As<std::optional<bool>>();
  parameters.supports_streaming = data["supports_streaming"].template As<std::optional<bool>>();
  parameters.disable_notification = data["disable_notification"].template As<std::optional<bool>>();
  parameters.protect_content = data["protect_content"].template As<std::optional<bool>>();
  parameters.reply_to_message_id = data["reply_to_message_id"].template As<std::optional<std::int64_t>>();
  parameters.allow_sending_without_reply = data["allow_sending_without_reply"].template As<std::optional<bool>>();
  parameters.reply_markup = data["reply_markup"].template As<std::optional<ReplyMarkup>>();
  return parameters;
}

template <class Value>
Value Serialize(const SendVideoMethod::Parameters& parameters, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["chat_id"] = parameters.chat_id;
  SetIfNotNull(builder, "message_thread_id", parameters.message_thread_id);
  builder["video"] = parameters.video;
  SetIfNotNull(builder, "duration", parameters.duration);
  SetIfNotNull(builder, "width", parameters.width);
  SetIfNotNull(builder, "height", parameters.height);
  SetIfNotNull(builder, "thumbnail", parameters.thumbnail);
  SetIfNotNull(builder, "caption", parameters.caption);
  SetIfNotNull(builder, "parse_mode", parameters.parse_mode);
  SetIfNotNull(builder, "caption_entities", parameters.caption_entities);
  SetIfNotNull(builder, "has_spoiler", parameters.has_spoiler);
  SetIfNotNull(builder, "supports_streaming", parameters.supports_streaming);
  SetIfNotNull(builder, "disable_notification", parameters.disable_notification);
  SetIfNotNull(builder, "protect_content", parameters.protect_content);
  SetIfNotNull(builder, "reply_to_message_id", parameters.reply_to_message_id);
  SetIfNotNull(builder, "allow_sending_without_reply", parameters.allow_sending_without_reply);
  SetIfNotNull(builder, "reply_markup", parameters.reply_markup);
  return builder.ExtractValue();
}

}  // namespace impl

SendVideoMethod::Parameters::Parameters(
    ChatId _chat_id,
    SendVideoMethod::Parameters::Video _video)
  : chat_id(std::move(_chat_id))
  , video(std::move(_video)) {}

void SendVideoMethod::FillRequestData(clients::http::Request& request,
                                      const Parameters& parameters) {
  if (!std::get_if<InputFile>(&parameters.video) && !parameters.thumbnail) {
    FillRequestDataAsJson<SendVideoMethod>(request, parameters);
    return;
  }

  clients::http::Form form;
  FillFormSection(form, "chat_id", parameters.chat_id);
  FillFormSection(form, "message_thread_id", parameters.message_thread_id);
  FillFormSection(form, "video", parameters.video);
  FillFormSection(form, "duration", parameters.duration);
  FillFormSection(form, "width", parameters.width);
  FillFormSection(form, "height", parameters.height);
  FillFormSection(form, "thumbnail", parameters.thumbnail);
  FillFormSection(form, "caption", parameters.caption);
  FillFormSection(form, "parse_mode", parameters.parse_mode);
  FillFormSection(form, "caption_entities", parameters.caption_entities);
  FillFormSection(form, "has_spoiler", parameters.has_spoiler);
  FillFormSection(form, "supports_streaming", parameters.supports_streaming);
  FillFormSection(form, "disable_notification", parameters.disable_notification);
  FillFormSection(form, "protect_content", parameters.protect_content);
  FillFormSection(form, "reply_to_message_id", parameters.reply_to_message_id);
  FillFormSection(form, "allow_sending_without_reply", parameters.allow_sending_without_reply);
  FillFormSection(form, "reply_markup", parameters.reply_markup);
  request.form(form);
}

SendVideoMethod::Reply SendVideoMethod::ParseResponseData(
    clients::http::Response& response) {
  return ParseResponseDataFromJson<SendVideoMethod>(response);
}

SendVideoMethod::Parameters Parse(
    const formats::json::Value& json,
    formats::parse::To<SendVideoMethod::Parameters> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(
    const SendVideoMethod::Parameters& parameters,
    formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(parameters, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
