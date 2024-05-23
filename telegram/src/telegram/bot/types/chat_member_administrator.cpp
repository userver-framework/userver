#include <userver/telegram/bot/types/chat_member_administrator.hpp>

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
ChatMemberAdministrator Parse(const Value& data,
                              formats::parse::To<ChatMemberAdministrator>) {
  std::string status = data["status"].template As<std::string>();
  UINVARIANT(status == ChatMemberAdministrator::kStatus,
             fmt::format("Invalid status for ChatMemberAdministrator : {}, expected: {}",
                         status, ChatMemberAdministrator::kStatus));

  return ChatMemberAdministrator{
    data["user"].template As<std::unique_ptr<User>>(),
    data["can_be_edited"].template As<bool>(),
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
    data["can_manage_topics"].template As<std::optional<bool>>(),
    data["custom_title"].template As<std::optional<std::string>>()
  };
}

template <class Value>
Value Serialize(const ChatMemberAdministrator& chat_member_administrator,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["status"] = ChatMemberAdministrator::kStatus;
  builder["user"] = chat_member_administrator.user;
  builder["can_be_edited"] = chat_member_administrator.can_be_edited;
  builder["is_anonymous"] = chat_member_administrator.is_anonymous;
  builder["can_manage_chat"] = chat_member_administrator.can_manage_chat;
  builder["can_delete_messages"] = chat_member_administrator.can_delete_messages;
  builder["can_manage_video_chats"] = chat_member_administrator.can_manage_video_chats;
  builder["can_restrict_members"] = chat_member_administrator.can_restrict_members;
  builder["can_promote_members"] = chat_member_administrator.can_promote_members;
  builder["can_change_info"] = chat_member_administrator.can_change_info;
  builder["can_invite_users"] = chat_member_administrator.can_invite_users;
  SetIfNotNull(builder, "can_post_messages", chat_member_administrator.can_post_messages);
  SetIfNotNull(builder, "can_edit_messages", chat_member_administrator.can_edit_messages);
  SetIfNotNull(builder, "can_pin_messages", chat_member_administrator.can_pin_messages);
  SetIfNotNull(builder, "can_post_stories", chat_member_administrator.can_post_stories);
  SetIfNotNull(builder, "can_edit_stories", chat_member_administrator.can_edit_stories);
  SetIfNotNull(builder, "can_delete_stories", chat_member_administrator.can_delete_stories);
  SetIfNotNull(builder, "can_manage_topics", chat_member_administrator.can_manage_topics);
  SetIfNotNull(builder, "custom_title", chat_member_administrator.custom_title);
  return builder.ExtractValue();
}

}  // namespace impl

ChatMemberAdministrator Parse(const formats::json::Value& json,
                              formats::parse::To<ChatMemberAdministrator> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const ChatMemberAdministrator& chat_member_administrator,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(chat_member_administrator, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
