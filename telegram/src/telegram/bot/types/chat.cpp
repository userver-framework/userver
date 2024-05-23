#include <userver/telegram/bot/types/chat.hpp>

#include <userver/telegram/bot/types/message.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>
#include <telegram/bot/formats/value_builder.hpp>
#include <telegram/bot/types/time.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize_duration.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/parse/common.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace {

constexpr utils::TrivialBiMap kChatTypeMap([](auto selector) {
  return selector()
    .Case(Chat::Type::kPrivate, "private")
    .Case(Chat::Type::kGroup, "group")
    .Case(Chat::Type::kSupergroup, "supergroup")
    .Case(Chat::Type::kChannel, "channel");
});

}  // namespace

namespace impl {

template <class Value>
Chat Parse(const Value& data, formats::parse::To<Chat>) {
  return Chat{
    data["id"].template As<std::int64_t>(),
    data["type"].template As<Chat::Type>(),
    data["title"].template As<std::optional<std::string>>(),
    data["username"].template As<std::optional<std::string>>(),
    data["first_name"].template As<std::optional<std::string>>(),
    data["last_name"].template As<std::optional<std::string>>(),
    data["is_forum"].template As<std::optional<bool>>(),
    data["photo"].template As<std::unique_ptr<ChatPhoto>>(),
    data["active_usernames"].template As<std::optional<std::vector<std::string>>>(),
    data["emoji_status_custom_emoji_id"].template As<std::optional<std::string>>(),
    TransformToTimePoint(data["emoji_status_expiration_date"].template As<std::optional<std::chrono::seconds>>()),
    data["bio"].template As<std::optional<std::string>>(),
    data["has_private_forwards"].template As<std::optional<bool>>(),
    data["has_restricted_voice_and_video_messages"].template As<std::optional<bool>>(),
    data["join_to_send_messages"].template As<std::optional<bool>>(),
    data["join_by_request"].template As<std::optional<bool>>(),
    data["description"].template As<std::optional<std::string>>(),
    data["invite_link"].template As<std::optional<std::string>>(),
    data["pinned_message"].template As<std::unique_ptr<Message>>(),
    data["permissions"].template As<std::unique_ptr<ChatPermissions>>(),
    data["slow_mode_delay"].template As<std::optional<std::chrono::seconds>>(),
    data["message_auto_delete_time"].template As<std::optional<std::chrono::seconds>>(),
    data["has_aggressive_anti_spam_enabled"].template As<std::optional<bool>>(),
    data["has_hidden_members"].template As<std::optional<bool>>(),
    data["has_protected_content"].template As<std::optional<bool>>(),
    data["sticker_set_name"].template As<std::optional<std::string>>(),
    data["can_set_sticker_set"].template As<std::optional<bool>>(),
    data["linked_chat_id"].template As<std::optional<std::int64_t>>(),
    data["location"].template As<std::unique_ptr<ChatLocation>>()
  };
}

template <class Value>
Value Serialize(const Chat& chat, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["id"] = chat.id;
  builder["type"] = chat.type;
  SetIfNotNull(builder, "title", chat.title);
  SetIfNotNull(builder, "username", chat.username);
  SetIfNotNull(builder, "first_name", chat.first_name);
  SetIfNotNull(builder, "last_name", chat.last_name);
  SetIfNotNull(builder, "is_forum", chat.is_forum);
  SetIfNotNull(builder, "photo", chat.photo);
  SetIfNotNull(builder, "active_usernames", chat.active_usernames);
  SetIfNotNull(builder, "emoji_status_custom_emoji_id", chat.emoji_status_custom_emoji_id);
  SetIfNotNull(builder, "emoji_status_expiration_date", TransformToSeconds(chat.emoji_status_expiration_date));
  SetIfNotNull(builder, "bio", chat.bio);
  SetIfNotNull(builder, "has_private_forwards", chat.has_private_forwards);
  SetIfNotNull(builder, "has_restricted_voice_and_video_messages", chat.has_restricted_voice_and_video_messages);
  SetIfNotNull(builder, "join_to_send_messages", chat.join_to_send_messages);
  SetIfNotNull(builder, "join_by_request", chat.join_by_request);
  SetIfNotNull(builder, "description", chat.description);
  SetIfNotNull(builder, "invite_link", chat.invite_link);
  SetIfNotNull(builder, "pinned_message", chat.pinned_message);
  SetIfNotNull(builder, "permissions", chat.permissions);
  SetIfNotNull(builder, "slow_mode_delay", chat.slow_mode_delay);
  SetIfNotNull(builder, "message_auto_delete_time", chat.message_auto_delete_time);
  SetIfNotNull(builder, "has_aggressive_anti_spam_enabled", chat.has_aggressive_anti_spam_enabled);
  SetIfNotNull(builder, "has_hidden_members", chat.has_hidden_members);
  SetIfNotNull(builder, "has_protected_content", chat.has_protected_content);
  SetIfNotNull(builder, "sticker_set_name", chat.sticker_set_name);
  SetIfNotNull(builder, "can_set_sticker_set", chat.can_set_sticker_set);
  SetIfNotNull(builder, "linked_chat_id", chat.linked_chat_id);
  SetIfNotNull(builder, "location", chat.location);
  return builder.ExtractValue();
}

}  // namespace impl

std::string_view ToString(Chat::Type chat_type) {
  return utils::impl::EnumToStringView(chat_type, kChatTypeMap);
}

Chat::Type Parse(const formats::json::Value& value,
                 formats::parse::To<Chat::Type>) {
  return utils::ParseFromValueString(value, kChatTypeMap);
}

formats::json::Value Serialize(Chat::Type chat_type,
                               formats::serialize::To<formats::json::Value>) {
  return formats::json::ValueBuilder(ToString(chat_type)).ExtractValue();
}

Chat Parse(const formats::json::Value& json,
           formats::parse::To<Chat> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const Chat& chat,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(chat, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
