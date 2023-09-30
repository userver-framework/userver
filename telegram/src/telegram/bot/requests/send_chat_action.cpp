#include <userver/telegram/bot/requests/send_chat_action.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>
#include <telegram/bot/formats/value_builder.hpp>
#include <telegram/bot/requests/request_data.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize_variant.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/parse/variant.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace {

constexpr utils::TrivialBiMap kActionMap([](auto selector) {
  return selector()
    .Case(SendChatActionMethod::Parameters::Action::kTyping, "typing")
    .Case(SendChatActionMethod::Parameters::Action::kUploadPhoto, "upload_photo")
    .Case(SendChatActionMethod::Parameters::Action::kRecordVideo, "record_video")
    .Case(SendChatActionMethod::Parameters::Action::kUploadVideo, "upload_video")
    .Case(SendChatActionMethod::Parameters::Action::kRecordVoice, "record_voice")
    .Case(SendChatActionMethod::Parameters::Action::kUploadVoice, "upload_voice")
    .Case(SendChatActionMethod::Parameters::Action::kChooseSticker, "choose_sticker")
    .Case(SendChatActionMethod::Parameters::Action::kFindLocation, "find_location")
    .Case(SendChatActionMethod::Parameters::Action::kRecordVideoNote, "record_video_note")
    .Case(SendChatActionMethod::Parameters::Action::kUploadVideoNote, "upload_video_note");
});

}  // namespace

namespace impl {

template <class Value>
SendChatActionMethod::Parameters Parse(
    const Value& data,
    formats::parse::To<SendChatActionMethod::Parameters>) {
  SendChatActionMethod::Parameters parameters{
    data["chat_id"].template As<ChatId>(),
    data["action"].template As<SendChatActionMethod::Parameters::Action>()
  };
  parameters.message_thread_id = data["message_thread_id"].template As<std::optional<std::int64_t>>();
  return parameters;
}

template <class Value>
Value Serialize(const SendChatActionMethod::Parameters& parameters,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["chat_id"] = parameters.chat_id;
  SetIfNotNull(builder, "message_thread_id", parameters.message_thread_id);
  builder["action"] = parameters.action;
  return builder.ExtractValue();
}

}  // namespace impl

SendChatActionMethod::Parameters::Parameters(
    ChatId _chat_id,
    SendChatActionMethod::Parameters::Action _action)
  : chat_id(std::move(_chat_id))
  , action(_action) {}

void SendChatActionMethod::FillRequestData(clients::http::Request& request,
                                           const Parameters& parameters) {
  FillRequestDataAsJson<SendChatActionMethod>(request, parameters);
}

SendChatActionMethod::Reply SendChatActionMethod::ParseResponseData(
    clients::http::Response& response) {
  return ParseResponseDataFromJson<SendChatActionMethod>(response);
}

std::string_view ToString(SendChatActionMethod::Parameters::Action action) {
  return utils::impl::EnumToStringView(action, kActionMap);
}

SendChatActionMethod::Parameters::Action Parse(
    const formats::json::Value& value,
    formats::parse::To<SendChatActionMethod::Parameters::Action>) {
  return utils::ParseFromValueString(value, kActionMap);
}

formats::json::Value Serialize(
    SendChatActionMethod::Parameters::Action action,
    formats::serialize::To<formats::json::Value>) {
  return formats::json::ValueBuilder(ToString(action)).ExtractValue();
}

SendChatActionMethod::Parameters Parse(
    const formats::json::Value& json,
    formats::parse::To<SendChatActionMethod::Parameters> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(
    const SendChatActionMethod::Parameters& parameters,
    formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(parameters, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
