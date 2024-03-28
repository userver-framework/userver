#include <userver/telegram/bot/types/chat_permissions.hpp>

#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
ChatPermissions Parse(const Value& data, formats::parse::To<ChatPermissions>) {
  return ChatPermissions{
    data["can_send_messages"].template As<std::optional<bool>>(),
    data["can_send_audios"].template As<std::optional<bool>>(),
    data["can_send_documents"].template As<std::optional<bool>>(),
    data["can_send_photos"].template As<std::optional<bool>>(),
    data["can_send_videos"].template As<std::optional<bool>>(),
    data["can_send_video_notes"].template As<std::optional<bool>>(),
    data["can_send_voice_notes"].template As<std::optional<bool>>(),
    data["can_send_polls"].template As<std::optional<bool>>(),
    data["can_send_other_messages"].template As<std::optional<bool>>(),
    data["can_add_web_page_previews"].template As<std::optional<bool>>(),
    data["can_change_info"].template As<std::optional<bool>>(),
    data["can_invite_users"].template As<std::optional<bool>>(),
    data["can_pin_messages"].template As<std::optional<bool>>(),
    data["can_manage_topics"].template As<std::optional<bool>>()
  };
}

template <class Value>
Value Serialize(const ChatPermissions& chat_permissions, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  SetIfNotNull(builder, "can_send_messages", chat_permissions.can_send_messages);
  SetIfNotNull(builder, "can_send_audios", chat_permissions.can_send_audios);
  SetIfNotNull(builder, "can_send_documents", chat_permissions.can_send_documents);
  SetIfNotNull(builder, "can_send_photos", chat_permissions.can_send_photos);
  SetIfNotNull(builder, "can_send_videos", chat_permissions.can_send_videos);
  SetIfNotNull(builder, "can_send_video_notes", chat_permissions.can_send_video_notes);
  SetIfNotNull(builder, "can_send_voice_notes", chat_permissions.can_send_voice_notes);
  SetIfNotNull(builder, "can_send_polls", chat_permissions.can_send_polls);
  SetIfNotNull(builder, "can_send_other_messages", chat_permissions.can_send_other_messages);
  SetIfNotNull(builder, "can_add_web_page_previews", chat_permissions.can_add_web_page_previews);
  SetIfNotNull(builder, "can_change_info", chat_permissions.can_change_info);
  SetIfNotNull(builder, "can_invite_users", chat_permissions.can_invite_users);
  SetIfNotNull(builder, "can_pin_messages", chat_permissions.can_pin_messages);
  SetIfNotNull(builder, "can_manage_topics", chat_permissions.can_manage_topics);
  return builder.ExtractValue();
}

}  // namespace impl

ChatPermissions Parse(const formats::json::Value& json,
                      formats::parse::To<ChatPermissions> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const ChatPermissions& chat_permissions,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(chat_permissions, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
