#include <userver/telegram/bot/types/chat_administrator_rights.hpp>

#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
ChatAdministratorRights Parse(const Value& data,
                              formats::parse::To<ChatAdministratorRights>) {
  return ChatAdministratorRights{
    data["is_anonymous"].template As<bool>(),
    data["can_manage_chat"].template As<bool>(),
    data["can_delete_messages"].template As<bool>(),
    data["can_manage_video_chats"].template As<bool>(),
    data["can_restrict_members"].template As<bool>(),
    data["can_promote_members"].template As<bool>(),
    data["can_change_info"].template As<bool>(),
    data["can_invite_users"].template As<bool>(),
    data["can_post_messages"].template As<std::optional<bool>>(),
    data["can_edit_messages"].template As<std::optional<bool>>(),
    data["can_pin_messages"].template As<std::optional<bool>>(),
    data["can_post_stories"].template As<std::optional<bool>>(),
    data["can_edit_stories"].template As<std::optional<bool>>(),
    data["can_delete_stories"].template As<std::optional<bool>>(),
    data["can_manage_topics"].template As<std::optional<bool>>()
  };
}

template <class Value>
Value Serialize(const ChatAdministratorRights& chat_administrator_rights,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["is_anonymous"] = chat_administrator_rights.is_anonymous;
  builder["can_manage_chat"] = chat_administrator_rights.can_manage_chat;
  builder["can_delete_messages"] = chat_administrator_rights.can_delete_messages;
  builder["can_manage_video_chats"] = chat_administrator_rights.can_manage_video_chats;
  builder["can_restrict_members"] = chat_administrator_rights.can_restrict_members;
  builder["can_promote_members"] = chat_administrator_rights.can_promote_members;
  builder["can_change_info"] = chat_administrator_rights.can_change_info;
  builder["can_invite_users"] = chat_administrator_rights.can_invite_users;
  SetIfNotNull(builder, "can_post_messages",chat_administrator_rights.can_post_messages);
  SetIfNotNull(builder, "can_edit_messages", chat_administrator_rights.can_edit_messages);
  SetIfNotNull(builder, "can_pin_messages", chat_administrator_rights.can_pin_messages);
  SetIfNotNull(builder, "can_post_stories", chat_administrator_rights.can_post_stories);
  SetIfNotNull(builder, "can_edit_stories", chat_administrator_rights.can_edit_stories);
  SetIfNotNull(builder, "can_delete_stories", chat_administrator_rights.can_delete_stories);
  SetIfNotNull(builder, "can_manage_topics", chat_administrator_rights.can_manage_topics);
  return builder.ExtractValue();
}

}  // namespace impl

ChatAdministratorRights Parse(const formats::json::Value& json,
                              formats::parse::To<ChatAdministratorRights> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const ChatAdministratorRights& chat_administrator_rights,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(chat_administrator_rights, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
