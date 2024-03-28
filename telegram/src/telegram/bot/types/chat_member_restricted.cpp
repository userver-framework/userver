#include <userver/telegram/bot/types/chat_member_restricted.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>
#include <telegram/bot/types/time.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize_duration.hpp>
#include <userver/formats/parse/common.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
ChatMemberRestricted Parse(const Value& data,
                           formats::parse::To<ChatMemberRestricted>) {
  std::string status = data["status"].template As<std::string>();
  UINVARIANT(status == ChatMemberRestricted::kStatus,
             fmt::format("Invalid status for ChatMemberRestricted: {}, expected: {}",
                         status, ChatMemberRestricted::kStatus));

  return ChatMemberRestricted{
    data["user"].template As<std::unique_ptr<User>>(),
    data["is_member"].template As<bool>(),
    data["can_send_messages"].template As<bool>(),
    data["can_send_audios"].template As<bool>(),
    data["can_send_documents"].template As<bool>(),
    data["can_send_photos"].template As<bool>(),
    data["can_send_videos"].template As<bool>(),
    data["can_send_video_notes"].template As<bool>(),
    data["can_send_voice_notes"].template As<bool>(),
    data["can_send_polls"].template As<bool>(),
    data["can_send_other_messages"].template As<bool>(),
    data["can_add_web_page_previews"].template As<bool>(),
    data["can_change_info"].template As<bool>(),
    data["can_invite_users"].template As<bool>(),
    data["can_pin_messages"].template As<bool>(),
    data["can_manage_topics"].template As<bool>(),
    TransformToTimePoint(data["until_date"].template As<std::chrono::seconds>())
  };
}

template <class Value>
Value Serialize(const ChatMemberRestricted& chat_member_restricted,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["status"] = ChatMemberRestricted::kStatus;
  builder["user"] = chat_member_restricted.user;
  builder["is_member"] = chat_member_restricted.is_member;
  builder["can_send_messages"] = chat_member_restricted.can_send_messages;
  builder["can_send_audios"] = chat_member_restricted.can_send_audios;
  builder["can_send_documents"] = chat_member_restricted.can_send_documents;
  builder["can_send_photos"] = chat_member_restricted.can_send_photos;
  builder["can_send_videos"] = chat_member_restricted.can_send_videos;
  builder["can_send_video_notes"] = chat_member_restricted.can_send_video_notes;
  builder["can_send_voice_notes"] = chat_member_restricted.can_send_voice_notes;
  builder["can_send_polls"] = chat_member_restricted.can_send_polls;
  builder["can_send_other_messages"] = chat_member_restricted.can_send_other_messages;
  builder["can_add_web_page_previews"] = chat_member_restricted.can_add_web_page_previews;
  builder["can_change_info"] = chat_member_restricted.can_change_info;
  builder["can_invite_users"] = chat_member_restricted.can_invite_users;
  builder["can_pin_messages"] = chat_member_restricted.can_pin_messages;
  builder["can_manage_topics"] = chat_member_restricted.can_manage_topics;
  builder["until_date"] = TransformToSeconds(chat_member_restricted.until_date);
  return builder.ExtractValue();
}

}  // namespace impl

ChatMemberRestricted Parse(const formats::json::Value& json,
                           formats::parse::To<ChatMemberRestricted> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const ChatMemberRestricted& chat_member_restricted,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(chat_member_restricted, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
