#include <userver/telegram/bot/requests/get_updates.hpp>

#include <telegram/bot/formats/value_builder.hpp>
#include <telegram/bot/requests/request_data.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize_duration.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/parse/common.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace {

constexpr utils::TrivialBiMap kUpdateTypeMap([](auto selector) {
  return selector()
    .Case(GetUpdatesMethod::Parameters::UpdateType::kMessage, "message")
    .Case(GetUpdatesMethod::Parameters::UpdateType::kEditedMessage, "edited_message")
    .Case(GetUpdatesMethod::Parameters::UpdateType::kChannelPost, "channel_post")
    .Case(GetUpdatesMethod::Parameters::UpdateType::kEditedChannelPost, "edited_channel_post")
    .Case(GetUpdatesMethod::Parameters::UpdateType::kInlineQuery, "inline_query")
    .Case(GetUpdatesMethod::Parameters::UpdateType::kChosenInlineResult, "chosen_inline_result")
    .Case(GetUpdatesMethod::Parameters::UpdateType::kCallbackQuery, "callback_query")
    .Case(GetUpdatesMethod::Parameters::UpdateType::kShippingQuery, "shipping_query")
    .Case(GetUpdatesMethod::Parameters::UpdateType::kPreCheckoutQuery, "pre_checkout_query")
    .Case(GetUpdatesMethod::Parameters::UpdateType::kPoll, "poll")
    .Case(GetUpdatesMethod::Parameters::UpdateType::kPollAnswer, "poll_answer")
    .Case(GetUpdatesMethod::Parameters::UpdateType::kMyChatMember, "my_chat_member")
    .Case(GetUpdatesMethod::Parameters::UpdateType::kChatMember, "chat_member")
    .Case(GetUpdatesMethod::Parameters::UpdateType::kChatJoinRequest, "chat_join_request");
});

}  // namespace

namespace impl {

template <class Value>
GetUpdatesMethod::Parameters Parse(const Value& data,
                                   formats::parse::To<GetUpdatesMethod::Parameters>) {
  return GetUpdatesMethod::Parameters{
    data["offset"].template As<std::optional<std::int64_t>>(),
    data["limit"].template As<std::optional<std::int64_t>>(),
    data["timeout"].template As<std::optional<std::chrono::seconds>>(),
    data["allowed_updates"].template As<std::optional<std::vector<GetUpdatesMethod::Parameters::UpdateType>>>()
  };
}

template <class Value>
Value Serialize(const GetUpdatesMethod::Parameters& parameters, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  SetIfNotNull(builder, "offset", parameters.offset);
  SetIfNotNull(builder, "limit", parameters.limit);
  SetIfNotNull(builder, "timeout", parameters.timeout);
  SetIfNotNull(builder, "allowed_updates", parameters.allowed_updates);
  return builder.ExtractValue();
}

}  // namespace impl

void GetUpdatesMethod::FillRequestData(clients::http::Request& request,
                                       const Parameters& parameters) {
  FillRequestDataAsJson<GetUpdatesMethod>(request, parameters);
}

GetUpdatesMethod::Reply GetUpdatesMethod::ParseResponseData(
    clients::http::Response& response) {
  return ParseResponseDataFromJson<GetUpdatesMethod>(response);
}

std::string_view ToString(GetUpdatesMethod::Parameters::UpdateType update_type) {
  return utils::impl::EnumToStringView(update_type, kUpdateTypeMap);
}

GetUpdatesMethod::Parameters::UpdateType Parse(
    const formats::json::Value& value,
    formats::parse::To<GetUpdatesMethod::Parameters::UpdateType>) {
  return utils::ParseFromValueString(value, kUpdateTypeMap);
}

formats::json::Value Serialize(
    GetUpdatesMethod::Parameters::UpdateType update_type,
    formats::serialize::To<formats::json::Value>) {
  return formats::json::ValueBuilder(ToString(update_type)).ExtractValue();
}

GetUpdatesMethod::Parameters Parse(const formats::json::Value& json,
                                   formats::parse::To<GetUpdatesMethod::Parameters> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const GetUpdatesMethod::Parameters& parameters,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(parameters, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
