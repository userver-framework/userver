#include <userver/telegram/bot/requests/send_document.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>
#include <telegram/bot/formats/value_builder.hpp>
#include <telegram/bot/requests/request_data.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize_variant.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/parse/variant.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
SendDocumentMethod::Parameters Parse(
    const Value& data,
    formats::parse::To<SendDocumentMethod::Parameters>) {
  SendDocumentMethod::Parameters parameters{
    data["chat_id"].template As<ChatId>(),
    data["document"].template As<SendDocumentMethod::Parameters::Document>()
  };
  parameters.message_thread_id = data["message_thread_id"].template As<std::optional<std::int64_t>>();
  parameters.thumbnail = data["thumbnail"].template As<std::unique_ptr<InputFile>>();
  parameters.caption = data["caption"].template As<std::optional<std::string>>();
  parameters.parse_mode = data["parse_mode"].template As<std::optional<std::string>>();
  parameters.caption_entities = data["caption_entities"].template As<std::optional<std::vector<MessageEntity>>>();
  parameters.disable_content_type_detection = data["disable_content_type_detection"].template As<std::optional<bool>>();
  parameters.disable_notification = data["disable_notification"].template As<std::optional<bool>>();
  parameters.protect_content = data["protect_content"].template As<std::optional<bool>>();
  parameters.reply_to_message_id = data["reply_to_message_id"].template As<std::optional<std::int64_t>>();
  parameters.allow_sending_without_reply = data["allow_sending_without_reply"].template As<std::optional<bool>>();
  parameters.reply_markup = data["reply_markup"].template As<std::optional<ReplyMarkup>>();
  return parameters;
}

template <class Value>
Value Serialize(const SendDocumentMethod::Parameters& parameters,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["chat_id"] = parameters.chat_id;
  SetIfNotNull(builder, "message_thread_id", parameters.message_thread_id);
  builder["document"] = parameters.document;
  SetIfNotNull(builder, "thumbnail", parameters.thumbnail);
  SetIfNotNull(builder, "caption", parameters.caption);
  SetIfNotNull(builder, "parse_mode", parameters.parse_mode);
  SetIfNotNull(builder, "caption_entities", parameters.caption_entities);
  SetIfNotNull(builder, "disable_content_type_detection", parameters.disable_content_type_detection);
  SetIfNotNull(builder, "disable_notification", parameters.disable_notification);
  SetIfNotNull(builder, "protect_content", parameters.protect_content);
  SetIfNotNull(builder, "reply_to_message_id", parameters.reply_to_message_id);
  SetIfNotNull(builder, "allow_sending_without_reply", parameters.allow_sending_without_reply);
  SetIfNotNull(builder, "reply_markup", parameters.reply_markup);
  return builder.ExtractValue();
}

}  // namespace impl

SendDocumentMethod::Parameters::Parameters(
    ChatId _chat_id,
    SendDocumentMethod::Parameters::Document _document)
  : chat_id(std::move(_chat_id))
  , document(std::move(_document)) {}

void SendDocumentMethod::FillRequestData(clients::http::Request& request,
                                         const Parameters& parameters) {
  if (!std::get_if<InputFile>(&parameters.document) && !parameters.thumbnail) {
    FillRequestDataAsJson<SendDocumentMethod>(request, parameters);
    return;
  }

  clients::http::Form form;
  FillFormSection(form, "chat_id", parameters.chat_id);
  FillFormSection(form, "message_thread_id", parameters.message_thread_id);
  FillFormSection(form, "document", parameters.document);
  FillFormSection(form, "thumbnail", parameters.thumbnail);
  FillFormSection(form, "caption", parameters.caption);
  FillFormSection(form, "parse_mode", parameters.parse_mode);
  FillFormSection(form, "caption_entities", parameters.caption_entities);
  FillFormSection(form, "disable_content_type_detection", parameters.disable_content_type_detection);
  FillFormSection(form, "disable_notification", parameters.disable_notification);
  FillFormSection(form, "protect_content", parameters.protect_content);
  FillFormSection(form, "reply_to_message_id", parameters.reply_to_message_id);
  FillFormSection(form, "allow_sending_without_reply", parameters.allow_sending_without_reply);
  FillFormSection(form, "reply_markup", parameters.reply_markup);
  request.form(form);
}

SendDocumentMethod::Reply SendDocumentMethod::ParseResponseData(
    clients::http::Response& response) {
  return ParseResponseDataFromJson<SendDocumentMethod>(response);
}

SendDocumentMethod::Parameters Parse(
    const formats::json::Value& json,
    formats::parse::To<SendDocumentMethod::Parameters> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(
    const SendDocumentMethod::Parameters& parameters,
    formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(parameters, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
