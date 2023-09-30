#include <userver/telegram/bot/requests/send_venue.hpp>

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
SendVenueMethod::Parameters Parse(
    const Value& data,
    formats::parse::To<SendVenueMethod::Parameters>) {
  SendVenueMethod::Parameters parameters{
    data["chat_id"].template As<ChatId>(),
    data["latitude"].template As<double>(),
    data["longitude"].template As<double>(),
    data["title"].template As<std::string>(),
    data["address"].template As<std::string>()
  };
  parameters.message_thread_id = data["message_thread_id"].template As<std::optional<std::int64_t>>();
  parameters.foursquare_id = data["foursquare_id"].template As<std::optional<std::string>>();
  parameters.foursquare_type = data["foursquare_type"].template As<std::optional<std::string>>();
  parameters.google_place_id = data["google_place_id"].template As<std::optional<std::string>>();
  parameters.google_place_type = data["google_place_type"].template As<std::optional<std::string>>();
  parameters.disable_notification = data["disable_notification"].template As<std::optional<bool>>();
  parameters.protect_content = data["protect_content"].template As<std::optional<bool>>();
  parameters.reply_to_message_id = data["reply_to_message_id"].template As<std::optional<std::int64_t>>();
  parameters.allow_sending_without_reply = data["allow_sending_without_reply"].template As<std::optional<bool>>();
  parameters.reply_markup = data["reply_markup"].template As<std::optional<ReplyMarkup>>();
  return parameters;
}

template <class Value>
Value Serialize(const SendVenueMethod::Parameters& parameters,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["chat_id"] = parameters.chat_id;
  SetIfNotNull(builder, "message_thread_id", parameters.message_thread_id);
  builder["latitude"] = parameters.latitude;
  builder["longitude"] = parameters.longitude;
  builder["title"] = parameters.title;
  builder["address"] = parameters.address;
  SetIfNotNull(builder, "foursquare_id", parameters.foursquare_id);
  SetIfNotNull(builder, "foursquare_type", parameters.foursquare_type);
  SetIfNotNull(builder, "google_place_id", parameters.google_place_id);
  SetIfNotNull(builder, "google_place_type", parameters.google_place_type);
  SetIfNotNull(builder, "disable_notification", parameters.disable_notification);
  SetIfNotNull(builder, "protect_content", parameters.protect_content);
  SetIfNotNull(builder, "reply_to_message_id", parameters.reply_to_message_id);
  SetIfNotNull(builder, "allow_sending_without_reply", parameters.allow_sending_without_reply);
  SetIfNotNull(builder, "reply_markup", parameters.reply_markup);
  return builder.ExtractValue();
}

}  // namespace impl

SendVenueMethod::Parameters::Parameters(ChatId _chat_id,
                                        double _latitude,
                                        double _longitude,
                                        std::string _title,
                                        std::string _address)
  : chat_id(std::move(_chat_id))
  , latitude(_latitude)
  , longitude(_longitude)
  , title(std::move(_title))
  , address(std::move(_address)) {}

void SendVenueMethod::FillRequestData(clients::http::Request& request,
                                      const Parameters& parameters) {
  FillRequestDataAsJson<SendVenueMethod>(request, parameters);
}

SendVenueMethod::Reply SendVenueMethod::ParseResponseData(
    clients::http::Response& response) {
  return ParseResponseDataFromJson<SendVenueMethod>(response);
}

SendVenueMethod::Parameters Parse(
    const formats::json::Value& json,
    formats::parse::To<SendVenueMethod::Parameters> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(
    const SendVenueMethod::Parameters& parameters,
    formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(parameters, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
