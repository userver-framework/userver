#include <userver/telegram/bot/types/chat_member_updated.hpp>

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
ChatMemberUpdated Parse(const Value& data,
                        formats::parse::To<ChatMemberUpdated>) {
  return ChatMemberUpdated{
    data["chat"].template As<std::unique_ptr<Chat>>(),
    data["from"].template As<std::unique_ptr<User>>(),
    TransformToTimePoint(data["date"].template As<std::chrono::seconds>()),
    data["old_chat_member"].template As<ChatMember>(),
    data["new_chat_member"].template As<ChatMember>(),
    data["invite_link"].template As<std::unique_ptr<ChatInviteLink>>(),
    data["via_chat_folder_invite_link"].template As<std::optional<bool>>()
  };
}

template <class Value>
Value Serialize(const ChatMemberUpdated& chat_member_updated,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["chat"] = chat_member_updated.chat;
  builder["from"] = chat_member_updated.from;
  builder["date"] = TransformToSeconds(chat_member_updated.date);
  builder["old_chat_member"] = chat_member_updated.old_chat_member;
  builder["new_chat_member"] = chat_member_updated.new_chat_member;
  SetIfNotNull(builder, "invite_link", chat_member_updated.invite_link);
  SetIfNotNull(builder, "via_chat_folder_invite_link", chat_member_updated.via_chat_folder_invite_link);
  return builder.ExtractValue();
}

}  // namespace impl

ChatMemberUpdated Parse(const formats::json::Value& json,
                        formats::parse::To<ChatMemberUpdated> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const ChatMemberUpdated& chat_member_updated,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(chat_member_updated, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
