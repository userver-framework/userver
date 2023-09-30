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
Message Parse(const Value& data, formats::parse::To<Message>) {
  return Message{
    data["message_id"].template As<std::int64_t>(),
    data["message_thread_id"].template As<std::optional<std::int64_t>>(),
    data["from"].template As<std::unique_ptr<User>>(),
    data["sender_chat"].template As<std::unique_ptr<Chat>>(),
    TransformToTimePoint(data["date"].template As<std::chrono::seconds>()),
    data["chat"].template As<std::unique_ptr<Chat>>(),
    data["forward_from"].template As<std::unique_ptr<User>>(),
    data["forward_from_chat"].template As<std::unique_ptr<Chat>>(),
    data["forward_from_message_id"].template As<std::optional<std::int64_t>>(),
    data["forward_signature"].template As<std::optional<std::string>>(),
    data["forward_sender_name"].template As<std::optional<std::string>>(),
    TransformToTimePoint(data["forward_date"].template As<std::optional<std::chrono::seconds>>()),
    data["is_topic_message"].template As<std::optional<bool>>(),
    data["is_automatic_forward"].template As<std::optional<bool>>(),
    data["reply_to_message"].template As<std::unique_ptr<Message>>(),
    data["via_bot"].template As<std::unique_ptr<User>>(),
    TransformToTimePoint(data["edit_date"].template As<std::optional<std::chrono::seconds>>()),
    data["has_protected_content"].template As<std::optional<bool>>(),
    data["media_group_id"].template As<std::optional<std::string>>(),
    data["author_signature"].template As<std::optional<std::string>>(),
    data["text"].template As<std::optional<std::string>>(),
    data["entities"].template As<std::optional<std::vector<MessageEntity>>>(),
    data["animation"].template As<std::unique_ptr<Animation>>(),
    data["audio"].template As<std::unique_ptr<Audio>>(),
    data["document"].template As<std::unique_ptr<Document>>(),
    data["photo"].template As<std::optional<std::vector<PhotoSize>>>(),
    data["sticker"].template As<std::unique_ptr<Sticker>>(),
    data["story"].template As<std::unique_ptr<Story>>(),
    data["video"].template As<std::unique_ptr<Video>>(),
    data["video_note"].template As<std::unique_ptr<VideoNote>>(),
    data["voice"].template As<std::unique_ptr<Voice>>(),
    data["caption"].template As<std::optional<std::string>>(),
    data["caption_entities"].template As<std::optional<std::vector<MessageEntity>>>(),
    data["has_media_spoiler"].template As<std::optional<bool>>(),
    data["contact"].template As<std::unique_ptr<Contact>>(),
    data["dice"].template As<std::unique_ptr<Dice>>(),
    data["game"].template As<std::unique_ptr<Game>>(),
    data["poll"].template As<std::unique_ptr<Poll>>(),
    data["venue"].template As<std::unique_ptr<Venue>>(),
    data["location"].template As<std::unique_ptr<Location>>(),
    data["new_chat_members"].template As<std::optional<std::vector<User>>>(),
    data["left_chat_member"].template As<std::unique_ptr<User>>(),
    data["new_chat_title"].template As<std::optional<std::string>>(),
    data["new_chat_photo"].template As<std::optional<std::vector<PhotoSize>>>(),
    data["delete_chat_photo"].template As<std::optional<bool>>(),
    data["group_chat_created"].template As<std::optional<bool>>(),
    data["supergroup_chat_created"].template As<std::optional<bool>>(),
    data["channel_chat_created"].template As<std::optional<bool>>(),
    data["message_auto_delete_timer_changed"].template As<std::unique_ptr<MessageAutoDeleteTimerChanged>>(),
    data["migrate_to_chat_id"].template As<std::optional<std::int64_t>>(),
    data["migrate_from_chat_id"].template As<std::optional<std::int64_t>>(),
    data["pinned_message"].template As<std::unique_ptr<Message>>(),
    data["invoice"].template As<std::unique_ptr<Invoice>>(),
    data["successful_payment"].template As<std::unique_ptr<SuccessfulPayment>>(),
    data["user_shared"].template As<std::unique_ptr<UserShared>>(),
    data["chat_shared"].template As<std::unique_ptr<ChatShared>>(),
    data["connected_website"].template As<std::optional<std::string>>(),
    data["write_access_allowed"].template As<std::unique_ptr<WriteAccessAllowed>>(),
    data["passport_data"].template As<std::unique_ptr<PassportData>>(),
    data["proximity_alert_triggered"].template As<std::unique_ptr<ProximityAlertTriggered>>(),
    data["forum_topic_created"].template As<std::unique_ptr<ForumTopicCreated>>(),
    data["forum_topic_edited"].template As<std::unique_ptr<ForumTopicEdited>>(),
    data["forum_topic_closed"].template As<std::unique_ptr<ForumTopicClosed>>(),
    data["forum_topic_reopened"].template As<std::unique_ptr<ForumTopicReopened>>(),
    data["general_forum_topic_hidden"].template As<std::unique_ptr<GeneralForumTopicHidden>>(),
    data["general_forum_topic_unhidden"].template As<std::unique_ptr<GeneralForumTopicUnhidden>>(),
    data["video_chat_scheduled"].template As<std::unique_ptr<VideoChatScheduled>>(),
    data["video_chat_started"].template As<std::unique_ptr<VideoChatStarted>>(),
    data["video_chat_ended"].template As<std::unique_ptr<VideoChatEnded>>(),
    data["video_chat_participants_invited"].template As<std::unique_ptr<VideoChatParticipantsInvited>>(),
    data["web_app_data"].template As<std::unique_ptr<WebAppData>>(),
    data["reply_markup"].template As<std::unique_ptr<InlineKeyboardMarkup>>()
  };
}

template <class Value>
Value Serialize(const Message& message, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["message_id"] = message.message_id;
  SetIfNotNull(builder, "message_thread_id", message.message_thread_id);
  SetIfNotNull(builder, "from", message.from);
  SetIfNotNull(builder, "sender_chat", message.sender_chat);
  builder["date"] = TransformToSeconds(message.date);
  builder["chat"] = message.chat;
  SetIfNotNull(builder, "forward_from", message.forward_from);
  SetIfNotNull(builder, "forward_from_chat", message.forward_from_chat);
  SetIfNotNull(builder, "forward_from_message_id", message.forward_from_message_id);
  SetIfNotNull(builder, "forward_signature", message.forward_signature);
  SetIfNotNull(builder, "forward_sender_name", message.forward_sender_name);
  SetIfNotNull(builder, "forward_date", TransformToSeconds(message.forward_date));
  SetIfNotNull(builder, "is_topic_message", message.is_topic_message);
  SetIfNotNull(builder, "is_automatic_forward", message.is_automatic_forward);
  SetIfNotNull(builder, "reply_to_message", message.reply_to_message);
  SetIfNotNull(builder, "via_bot", message.via_bot);
  SetIfNotNull(builder, "edit_date", TransformToSeconds(message.edit_date));
  SetIfNotNull(builder, "has_protected_content", message.has_protected_content);
  SetIfNotNull(builder, "media_group_id", message.media_group_id);
  SetIfNotNull(builder, "author_signature", message.author_signature);
  SetIfNotNull(builder, "text", message.text);
  SetIfNotNull(builder, "entities", message.entities);
  SetIfNotNull(builder, "animation", message.animation);
  SetIfNotNull(builder, "audio", message.audio);
  SetIfNotNull(builder, "document", message.document);
  SetIfNotNull(builder, "photo", message.photo);
  SetIfNotNull(builder, "sticker", message.sticker);
  SetIfNotNull(builder, "story", message.story);
  SetIfNotNull(builder, "video", message.video);
  SetIfNotNull(builder, "video_note", message.video_note);
  SetIfNotNull(builder, "voice", message.voice);
  SetIfNotNull(builder, "caption", message.caption);
  SetIfNotNull(builder, "caption_entities", message.caption_entities);
  SetIfNotNull(builder, "has_media_spoiler", message.has_media_spoiler);
  SetIfNotNull(builder, "contact", message.contact);
  SetIfNotNull(builder, "dice", message.dice);
  SetIfNotNull(builder, "game", message.game);
  SetIfNotNull(builder, "poll", message.poll);
  SetIfNotNull(builder, "venue", message.venue);
  SetIfNotNull(builder, "location", message.location);
  SetIfNotNull(builder, "new_chat_members", message.new_chat_members);
  SetIfNotNull(builder, "left_chat_member", message.left_chat_member);
  SetIfNotNull(builder, "new_chat_title", message.new_chat_title);
  SetIfNotNull(builder, "new_chat_photo", message.new_chat_photo);
  SetIfNotNull(builder, "delete_chat_photo", message.delete_chat_photo);
  SetIfNotNull(builder, "group_chat_created", message.group_chat_created);
  SetIfNotNull(builder, "supergroup_chat_created", message.supergroup_chat_created);
  SetIfNotNull(builder, "channel_chat_created", message.channel_chat_created);
  SetIfNotNull(builder, "message_auto_delete_timer_changed", message.message_auto_delete_timer_changed);
  SetIfNotNull(builder, "migrate_to_chat_id", message.migrate_to_chat_id);
  SetIfNotNull(builder, "migrate_from_chat_id", message.migrate_from_chat_id);
  SetIfNotNull(builder, "pinned_message", message.pinned_message);
  SetIfNotNull(builder, "invoice", message.invoice);
  SetIfNotNull(builder, "successful_payment", message.successful_payment);
  SetIfNotNull(builder, "user_shared", message.user_shared);
  SetIfNotNull(builder, "chat_shared", message.chat_shared);
  SetIfNotNull(builder, "connected_website", message.connected_website);
  SetIfNotNull(builder, "write_access_allowed", message.write_access_allowed);
  SetIfNotNull(builder, "passport_data", message.passport_data);
  SetIfNotNull(builder, "proximity_alert_triggered", message.proximity_alert_triggered);
  SetIfNotNull(builder, "forum_topic_created", message.forum_topic_created);
  SetIfNotNull(builder, "forum_topic_edited", message.forum_topic_edited);
  SetIfNotNull(builder, "forum_topic_closed", message.forum_topic_closed);
  SetIfNotNull(builder, "forum_topic_reopened", message.forum_topic_reopened);
  SetIfNotNull(builder, "general_forum_topic_hidden", message.general_forum_topic_hidden);
  SetIfNotNull(builder, "general_forum_topic_unhidden", message.general_forum_topic_unhidden);
  SetIfNotNull(builder, "video_chat_scheduled", message.video_chat_scheduled);
  SetIfNotNull(builder, "video_chat_started", message.video_chat_started);
  SetIfNotNull(builder, "video_chat_ended", message.video_chat_ended);
  SetIfNotNull(builder, "video_chat_participants_invited", message.video_chat_participants_invited);
  SetIfNotNull(builder, "web_app_data", message.web_app_data);
  SetIfNotNull(builder, "reply_markup", message.reply_markup);
  return builder.ExtractValue();
}

}  // namespace impl

Message Parse(const formats::json::Value& json,
              formats::parse::To<Message> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const Message& message,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(message, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
