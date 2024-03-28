#pragma once

#include <userver/telegram/bot/types/animation.hpp>
#include <userver/telegram/bot/types/audio.hpp>
#include <userver/telegram/bot/types/chat.hpp>
#include <userver/telegram/bot/types/chat_shared.hpp>
#include <userver/telegram/bot/types/contact.hpp>
#include <userver/telegram/bot/types/dice.hpp>
#include <userver/telegram/bot/types/document.hpp>
#include <userver/telegram/bot/types/forum_topic_closed.hpp>
#include <userver/telegram/bot/types/forum_topic_created.hpp>
#include <userver/telegram/bot/types/forum_topic_edited.hpp>
#include <userver/telegram/bot/types/forum_topic_reopened.hpp>
#include <userver/telegram/bot/types/game.hpp>
#include <userver/telegram/bot/types/general_forum_topic_hidden.hpp>
#include <userver/telegram/bot/types/general_forum_topic_unhidden.hpp>
#include <userver/telegram/bot/types/inline_keyboard_markup.hpp>
#include <userver/telegram/bot/types/invoice.hpp>
#include <userver/telegram/bot/types/location.hpp>
#include <userver/telegram/bot/types/message_auto_delete_timer_changed.hpp>
#include <userver/telegram/bot/types/message_entity.hpp>
#include <userver/telegram/bot/types/passport_data.hpp>
#include <userver/telegram/bot/types/photo_size.hpp>
#include <userver/telegram/bot/types/poll.hpp>
#include <userver/telegram/bot/types/proximity_alert_triggered.hpp>
#include <userver/telegram/bot/types/sticker.hpp>
#include <userver/telegram/bot/types/story.hpp>
#include <userver/telegram/bot/types/successful_payment.hpp>
#include <userver/telegram/bot/types/user.hpp>
#include <userver/telegram/bot/types/user_shared.hpp>
#include <userver/telegram/bot/types/venue.hpp>
#include <userver/telegram/bot/types/video.hpp>
#include <userver/telegram/bot/types/video_chat_ended.hpp>
#include <userver/telegram/bot/types/video_chat_participants_invited.hpp>
#include <userver/telegram/bot/types/video_chat_scheduled.hpp>
#include <userver/telegram/bot/types/video_chat_started.hpp>
#include <userver/telegram/bot/types/video_note.hpp>
#include <userver/telegram/bot/types/voice.hpp>
#include <userver/telegram/bot/types/web_app_data.hpp>
#include <userver/telegram/bot/types/write_access_allowed.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a message.
/// @see https://core.telegram.org/bots/api#message
struct Message {
  /// @brief Unique message identifier inside this chat.
  std::int64_t message_id{};

  /// @brief Optional. Unique identifier of a message thread to which
  /// the message belongs.
  /// @note For supergroups only.
  std::optional<std::int64_t> message_thread_id;

  /// @brief Optional. Sender of the message; empty for messages
  /// sent to channels.
  /// @note For backward compatibility, the field contains a fake sender
  /// user in non-channel chats, if the message was sent on behalf of a chat.
  std::unique_ptr<User> from;

  /// @brief Optional. Sender of the message, sent on behalf of a chat.
  /// For example, the channel itself for channel posts, the supergroup itself
  /// for messages from anonymous group administrators, the linked channel for
  /// messages automatically forwarded to the discussion group.
  /// @note For backward compatibility, the field from contains a fake sender
  /// user in non-channel chats, if the message was sent on behalf of a chat.
  std::unique_ptr<Chat> sender_chat;

  /// @brief Date the message was sent in Unix time.
  std::chrono::system_clock::time_point date{};

  /// @brief Conversation the message belongs to.
  std::unique_ptr<Chat> chat;

  /// @brief Optional. Sender of the original message.
  /// @note For forwarded messages.
  std::unique_ptr<User> forward_from;

  /// @brief Optional. Information about the original sender chat.
  /// @note For messages forwarded from channels or from
  /// anonymous administrators.
  std::unique_ptr<Chat> forward_from_chat;

  /// @brief Optional. Identifier of the original message in the channel.
  /// @note For messages forwarded from channels.
  std::optional<std::int64_t> forward_from_message_id;

  /// @brief Optional. Signature of the message sender if present.
  /// @note For forwarded messages that were originally sent in channels
  /// or by an anonymous chat administrator.
  std::optional<std::string> forward_signature;

  /// @brief Optional. Sender's name.
  /// @note For messages forwarded from users who disallow adding
  /// a link to their account in forwarded messages.
  std::optional<std::string> forward_sender_name;

  /// @brief Optional. Date the original message was sent in Unix time.
  /// @note For forwarded messages.
  std::optional<std::chrono::system_clock::time_point> forward_date;

  /// @brief Optional. True, if the message is sent to a forum topic.
  std::optional<bool> is_topic_message;

  /// @brief Optional. True, if the message is a channel post that was
  /// automatically forwarded to the connected discussion group.
  std::optional<bool> is_automatic_forward;

  /// @brief Optional. The original message.
  /// @note For replies.
  /// @note The Message object in this field will not contain further
  /// reply_to_message fields even if it itself is a reply.
  std::unique_ptr<Message> reply_to_message;

  /// @brief Optional. Bot through which the message was sent.
  std::unique_ptr<User> via_bot;

  /// @brief Optional. Date the message was last edited in Unix time.
  std::optional<std::chrono::system_clock::time_point> edit_date;

  /// @brief Optional. True, if the message can't be forwarded.
  std::optional<bool> has_protected_content;

  /// @brief Optional. The unique identifier of a media message group
  /// this message belongs to.
  std::optional<std::string> media_group_id;

  /// @brief Optional. Signature of the post author for messages in channels,
  /// or the custom title of an anonymous group administrator.
  std::optional<std::string> author_signature;

  /// @brief Optional. For text messages, the actual UTF-8 text of the message.
  std::optional<std::string> text;

  /// @brief Optional. For text messages, special entities like usernames,
  /// URLs, bot commands, etc. that appear in the text.
  std::optional<std::vector<MessageEntity>> entities;

  /// @brief Optional. Message is an animation, information about the animation.
  /// @note For backward compatibility, when this field is set,
  /// the document field will also be set.
  std::unique_ptr<Animation> animation;

  /// @brief Optional. Message is an audio file, information about the file.
  std::unique_ptr<Audio> audio;

  /// @brief Optional. Message is a general file, information about the file.
  std::unique_ptr<Document> document;

  /// @brief Optional. Message is a photo, available sizes of the photo.
  std::optional<std::vector<PhotoSize>> photo;

  /// @brief Optional. Message is a sticker, information about the sticker.
  std::unique_ptr<Sticker> sticker;

  /// @brief Optional. Message is a forwarded story.
  std::unique_ptr<Story> story;

  /// @brief Optional. Message is a video, information about the video.
  std::unique_ptr<Video> video;

  /// @brief Optional. Message is a video note, information about
  /// the video message.
  std::unique_ptr<VideoNote> video_note;

  /// @brief Optional. Message is a voice message, information about the file.
  std::unique_ptr<Voice> voice;

  /// @brief Optional. Caption for the animation, audio, document, photo,
  /// video or voice.
  std::optional<std::string> caption;

  /// @brief Optional. For messages with a caption, special entities like
  /// usernames, URLs, bot commands, etc. that appear in the caption.
  std::optional<std::vector<MessageEntity>> caption_entities;

  /// @brief Optional. True, if the message media is covered by a
  /// spoiler animation.
  std::optional<bool> has_media_spoiler;

  /// @brief Optional. Message is a shared contact, information about
  /// the contact.
  std::unique_ptr<Contact> contact;

  /// @brief Optional. Message is a dice with random value.
  std::unique_ptr<Dice> dice;

  /// @brief Optional. Message is a game, information about the game.
  /// @see https://core.telegram.org/bots/api#games
  std::unique_ptr<Game> game;

  /// @brief Optional. Message is a native poll, information about the poll.
  std::unique_ptr<Poll> poll;

  /// @brief Optional. Message is a venue, information about the venue.
  /// @note For backward compatibility, when this field is set,
  /// the location field will also be set.
  std::unique_ptr<Venue> venue;

  /// @brief Optional. Message is a shared location, information about
  /// the location.
  std::unique_ptr<Location> location;

  /// @brief Optional. New members that were added to the group or supergroup
  /// and information about them (the bot itself may be one of these members).
  std::optional<std::vector<User>> new_chat_members;

  /// @brief Optional. A member was removed from the group, information about
  /// them (this member may be the bot itself).
  std::unique_ptr<User> left_chat_member;

  /// @brief Optional. A chat title was changed to this value.
  std::optional<std::string> new_chat_title;

  /// @brief Optional. A chat photo was change to this value.
  std::optional<std::vector<PhotoSize>> new_chat_photo;

  /// @brief Optional. Service message: the chat photo was deleted.
  std::optional<bool> delete_chat_photo;

  /// @brief Optional. Service message: the group has been created.
  std::optional<bool> group_chat_created;

  /// @brief Optional. Service message: the supergroup has been created.
  /// @note This field can't be received in a message coming through updates,
  /// because bot can't be a member of a supergroup when it is created.
  /// It can only be found in reply_to_message if someone replies to a very
  /// first message in a directly created supergroup.
  std::optional<bool> supergroup_chat_created;

  /// @brief Optional. Service message: the channel has been created.
  /// @note This field can't be received in a message coming through updates,
  /// because bot can't be a member of a channel when it is created.
  /// It can only be found in reply_to_message if someone replies to
  /// a very first message in a channel.
  std::optional<bool> channel_chat_created;

  /// @brief Optional. Service message: auto-delete timer settings
  /// changed in the chat.
  std::unique_ptr<MessageAutoDeleteTimerChanged>
    message_auto_delete_timer_changed;

  /// @brief Optional. The group has been migrated to a supergroup with the
  /// specified identifier.
  std::optional<std::int64_t> migrate_to_chat_id;

  /// @brief Optional. The supergroup has been migrated from a group with the
  /// specified identifier.
  std::optional<std::int64_t> migrate_from_chat_id;

  /// @brief Optional. Specified message was pinned.
  /// @note The Message object in this field will not contain further
  /// reply_to_message fields even if it is itself a reply.
  std::unique_ptr<Message> pinned_message;

  /// @brief Optional. Message is an invoice for a payment, information
  /// about the invoice.
  /// @see https://core.telegram.org/bots/api#payments
  std::unique_ptr<Invoice> invoice;

  /// @brief Optional. Message is a service message about a successful payment,
  /// information about the payment.
  /// @see https://core.telegram.org/bots/api#payments
  std::unique_ptr<SuccessfulPayment> successful_payment;

  /// @brief Optional. Service message: a user was shared with the bot.
  std::unique_ptr<UserShared> user_shared;

  /// @brief Optional. Service message: a chat was shared with the bot.
  std::unique_ptr<ChatShared> chat_shared;

  /// @brief Optional. The domain name of the website on which the user
  // has logged in.
  /// @see https://core.telegram.org/widgets/login
  std::optional<std::string> connected_website;

  /// @brief Optional. Service message: the user allowed the bot to write
  /// messages after adding it to the attachment or side menu, launching
  /// a Web App from a link, or accepting an explicit request from a Web App
  /// sent by the method RequestWriteAccess.
  std::unique_ptr<WriteAccessAllowed> write_access_allowed;

  /// @brief Optional. Telegram Passport data.
  std::unique_ptr<PassportData> passport_data;

  /// @brief Optional. Service message. A user in the chat triggered another
  /// user's proximity alert while sharing Live Location.
  std::unique_ptr<ProximityAlertTriggered> proximity_alert_triggered;

  /// @brief Optional. Service message: forum topic created.
  std::unique_ptr<ForumTopicCreated> forum_topic_created;

  /// @brief Optional. Service message: forum topic edited.
  std::unique_ptr<ForumTopicEdited> forum_topic_edited;

  /// @brief Optional. Service message: forum topic closed.
  std::unique_ptr<ForumTopicClosed> forum_topic_closed;

  /// @brief Optional. Service message: forum topic reopened.
  std::unique_ptr<ForumTopicReopened> forum_topic_reopened;

  /// @brief Optional. Service message: the 'General' forum topic hidden.
  std::unique_ptr<GeneralForumTopicHidden> general_forum_topic_hidden;

  /// @brief Optional. Service message: the 'General' forum topic unhidden.
  std::unique_ptr<GeneralForumTopicUnhidden> general_forum_topic_unhidden;

  /// @brief Optional. Service message: video chat scheduled.
  std::unique_ptr<VideoChatScheduled> video_chat_scheduled;

  /// @brief Optional. Service message: video chat started.
  std::unique_ptr<VideoChatStarted> video_chat_started;

  /// @brief Optional. Service message: video chat ended.
  std::unique_ptr<VideoChatEnded> video_chat_ended;

  /// @brief Optional. Service message: new participants invited to a
  /// video chat.
  std::unique_ptr<VideoChatParticipantsInvited> video_chat_participants_invited;

  /// @brief Optional. Service message: data sent by a Web App.
  std::unique_ptr<WebAppData> web_app_data;

  /// @brief Optional. Inline keyboard attached to the message.
  /// login_url buttons are represented as ordinary url buttons.
  std::unique_ptr<InlineKeyboardMarkup> reply_markup;
};

Message Parse(const formats::json::Value& json,
              formats::parse::To<Message>);

formats::json::Value Serialize(const Message& message,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
