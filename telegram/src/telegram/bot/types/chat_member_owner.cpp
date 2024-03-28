#include <userver/telegram/bot/types/chat_member_owner.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>
#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
ChatMemberOwner Parse(const Value& data, formats::parse::To<ChatMemberOwner>) {
  std::string status = data["status"].template As<std::string>();
  UINVARIANT(status == ChatMemberOwner::kStatus,
             fmt::format("Invalid status for ChatMemberOwner: {}, expected: {}",
                         status, ChatMemberOwner::kStatus));

  return ChatMemberOwner{
    data["user"].template As<std::unique_ptr<User>>(),
    data["is_anonymous"].template As<bool>(),
    data["custom_title"].template As<std::optional<std::string>>()
  };
}

template <class Value>
Value Serialize(const ChatMemberOwner& chat_member_owner,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["status"] = ChatMemberOwner::kStatus;
  builder["user"] = chat_member_owner.user;
  builder["is_anonymous"] = chat_member_owner.is_anonymous;
  SetIfNotNull(builder, "custom_title", chat_member_owner.custom_title);
  return builder.ExtractValue();
}

}  // namespace impl

ChatMemberOwner Parse(const formats::json::Value& json,
                      formats::parse::To<ChatMemberOwner> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const ChatMemberOwner& chat_member_owner,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(chat_member_owner, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
