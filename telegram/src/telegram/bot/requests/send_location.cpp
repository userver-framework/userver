#include <userver/telegram/bot/requests/send_location.hpp>

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
SendLocationMethod::Parameters Parse(
    const Value& data,
    formats::parse::To<SendLocationMethod::Parameters>) {
  SendLocationMethod::Parameters parameters{
    data["chat_id"].template As<ChatId>(),
    data["latitude"].template As<double>(),
    data["longitude"].template As<double>()
  };
  parameters.message_thread_id = data["message_thread_id"].template As<std::optional<std::int64_t>>();
  parameters.horizontal_accuracy = data["horizontal_accuracy"].template As<std::optional<double>>();
  parameters.live_period = data["live_period"].template As<std::optional<std::int64_t>>();
  parameters.heading = data["heading"].template As<std::optional<std::int64_t>>();
  parameters.proximity_alert_radius = data["proximity_alert_radius"].template As<std::optional<std::int64_t>>();
  parameters.disable_notification = data["disable_notification"].template As<std::optional<bool>>();
  parameters.protect_content = data["protect_content"].template As<std::optional<bool>>();
  parameters.reply_to_message_id = data["reply_to_message_id"].template As<std::optional<std::int64_t>>();
  parameters.allow_sending_without_reply = data["allow_sending_without_reply"].template As<std::optional<bool>>();
  parameters.reply_markup = data["reply_markup"].template As<std::optional<ReplyMarkup>>();
  return parameters;
}

template <class Value>
Value Serialize(const SendLocationMethod::Parameters& parameters, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["chat_id"] = parameters.chat_id;
  SetIfNotNull(builder, "message_thread_id", parameters.message_thread_id);
  builder["latitude"] = parameters.latitude;
  builder["longitude"] = parameters.longitude;
  SetIfNotNull(builder, "horizontal_accuracy", parameters.horizontal_accuracy);
  SetIfNotNull(builder, "live_period", parameters.live_period);
  SetIfNotNull(builder, "heading", parameters.heading);
  SetIfNotNull(builder, "proximity_alert_radius", parameters.proximity_alert_radius);
  SetIfNotNull(builder, "disable_notification", parameters.disable_notification);
  SetIfNotNull(builder, "protect_content", parameters.protect_content);
  SetIfNotNull(builder, "reply_to_message_id", parameters.reply_to_message_id);
  SetIfNotNull(builder, "allow_sending_without_reply", parameters.allow_sending_without_reply);
  SetIfNotNull(builder, "reply_markup", parameters.reply_markup);
  return builder.ExtractValue();
}

}  // namespace impl

SendLocationMethod::Parameters::Parameters(ChatId _chat_id,
                                           double _latitude,
                                           double _longitude)
  : chat_id(std::move(_chat_id))
  , latitude(_latitude)
  , longitude(_longitude) {}

void SendLocationMethod::FillRequestData(clients::http::Request& request,
                                         const Parameters& parameters) {
  FillRequestDataAsJson<SendLocationMethod>(request, parameters);
}

SendLocationMethod::Reply SendLocationMethod::ParseResponseData(
    clients::http::Response& response) {
  return ParseResponseDataFromJson<SendLocationMethod>(response);
}

SendLocationMethod::Parameters Parse(
    const formats::json::Value& json,
    formats::parse::To<SendLocationMethod::Parameters> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(
    const SendLocationMethod::Parameters& parameters,
    formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(parameters, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
