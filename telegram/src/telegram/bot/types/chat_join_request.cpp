#include <userver/telegram/bot/types/chat_join_request.hpp>

#include <userver/telegram/bot/types/message.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>
#include <telegram/bot/formats/value_builder.hpp>
#include <telegram/bot/types/time.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize_duration.hpp>
#include <userver/formats/parse/common.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
ChatJoinRequest Parse(const Value& data, formats::parse::To<ChatJoinRequest>) {
  return ChatJoinRequest{
    data["chat"].template As<std::unique_ptr<Chat>>(),
    data["from"].template As<std::unique_ptr<User>>(),
    data["user_chat_id"].template As<std::int64_t>(),
    TransformToTimePoint(data["date"].template As<std::chrono::seconds>()),
    data["bio"].template As<std::optional<std::string>>(),
    data["invite_link"].template As<std::unique_ptr<ChatInviteLink>>()
  };
}

template <class Value>
Value Serialize(const ChatJoinRequest& chat_join_request,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["chat"] = chat_join_request.chat;
  builder["from"] = chat_join_request.from;
  builder["user_chat_id"] = chat_join_request.user_chat_id;
  builder["date"] = TransformToSeconds(chat_join_request.date);
  SetIfNotNull(builder, "bio", chat_join_request.bio);
  SetIfNotNull(builder, "invite_link", chat_join_request.invite_link);
  return builder.ExtractValue();
}

}  // namespace impl

ChatJoinRequest Parse(const formats::json::Value& json,
                      formats::parse::To<ChatJoinRequest> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const ChatJoinRequest& chat_join_request,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(chat_join_request, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
