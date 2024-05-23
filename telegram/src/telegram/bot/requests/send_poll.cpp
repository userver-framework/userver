#include <userver/telegram/bot/requests/send_poll.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>
#include <telegram/bot/formats/value_builder.hpp>
#include <telegram/bot/requests/request_data.hpp>
#include <telegram/bot/types/time.hpp>

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
SendPollMethod::Parameters Parse(
    const Value& data,
    formats::parse::To<SendPollMethod::Parameters>) {
  SendPollMethod::Parameters parameters{
    data["chat_id"].template As<ChatId>(),
    data["question"].template As<std::string>(),
    data["options"].template As<std::vector<std::string>>()
  };
  parameters.message_thread_id = data["message_thread_id"].template As<std::optional<std::int64_t>>();
  parameters.is_anonymous = data["is_anonymous"].template As<std::optional<bool>>();
  parameters.type = data["type"].template As<std::optional<Poll::Type>>();
  parameters.allows_multiple_answers = data["allows_multiple_answers"].template As<std::optional<bool>>();
  parameters.correct_option_id = data["correct_option_id"].template As<std::optional<std::int64_t>>();
  parameters.explanation = data["explanation"].template As<std::optional<std::string>>();
  parameters.explanation_parse_mode = data["explanation_parse_mode"].template As<std::optional<std::string>>();
  parameters.explanation_entities = data["explanation_entities"].template As<std::optional<std::vector<MessageEntity>>>();
  parameters.open_period = data["open_period"].template As<std::optional<std::chrono::seconds>>();
  parameters.close_date = TransformToTimePoint(data["close_date"].template As<std::optional<std::chrono::seconds>>());
  parameters.is_closed = data["is_closed"].template As<std::optional<bool>>();
  parameters.disable_notification = data["disable_notification"].template As<std::optional<bool>>();
  parameters.protect_content = data["protect_content"].template As<std::optional<bool>>();
  parameters.reply_to_message_id = data["reply_to_message_id"].template As<std::optional<std::int64_t>>();
  parameters.allow_sending_without_reply = data["allow_sending_without_reply"].template As<std::optional<bool>>();
  parameters.reply_markup = data["reply_markup"].template As<std::optional<ReplyMarkup>>();
  return parameters;
}

template <class Value>
Value Serialize(const SendPollMethod::Parameters& parameters, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["chat_id"] = parameters.chat_id;
  SetIfNotNull(builder, "message_thread_id", parameters.message_thread_id);
  builder["question"] = parameters.question;
  builder["options"] = parameters.options;
  SetIfNotNull(builder, "is_anonymous", parameters.is_anonymous);
  SetIfNotNull(builder, "type", parameters.type);
  SetIfNotNull(builder, "allows_multiple_answers", parameters.allows_multiple_answers);
  SetIfNotNull(builder, "correct_option_id", parameters.correct_option_id);
  SetIfNotNull(builder, "explanation", parameters.explanation);
  SetIfNotNull(builder, "explanation_parse_mode", parameters.explanation_parse_mode);
  SetIfNotNull(builder, "explanation_entities", parameters.explanation_entities);
  SetIfNotNull(builder, "open_period", parameters.open_period);
  SetIfNotNull(builder, "close_date", TransformToSeconds(parameters.close_date));
  SetIfNotNull(builder, "is_closed", parameters.is_closed);
  SetIfNotNull(builder, "disable_notification", parameters.disable_notification);
  SetIfNotNull(builder, "protect_content", parameters.protect_content);
  SetIfNotNull(builder, "reply_to_message_id", parameters.reply_to_message_id);
  SetIfNotNull(builder, "allow_sending_without_reply", parameters.allow_sending_without_reply);
  SetIfNotNull(builder, "reply_markup", parameters.reply_markup);
  return builder.ExtractValue();
}

}  // namespace impl

SendPollMethod::Parameters::Parameters(ChatId _chat_id,
                                       std::string _question,
                                       std::vector<std::string> _options)
  : chat_id(std::move(_chat_id))
  , question(std::move(_question))
  , options(std::move(_options)) {}

void SendPollMethod::FillRequestData(clients::http::Request& request,
                                     const Parameters& parameters) {
  FillRequestDataAsJson<SendPollMethod>(request, parameters);
}

SendPollMethod::Reply SendPollMethod::ParseResponseData(
    clients::http::Response& response) {
  return ParseResponseDataFromJson<SendPollMethod>(response);
}

SendPollMethod::Parameters Parse(
    const formats::json::Value& json,
    formats::parse::To<SendPollMethod::Parameters> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(
    const SendPollMethod::Parameters& parameters,
    formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(parameters, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
