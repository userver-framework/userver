#pragma once

#include <userver/telegram/bot/types/user.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents one special entity in a text message.
/// For example, hashtags, usernames, URLs, etc.
/// @see https://core.telegram.org/bots/api#messageentity
struct MessageEntity {
  enum class Type {
    kMention,       ///< For example, @username
    kHashtag,       ///< For example, #hashtag
    kCashtag,       ///< For example, $USD
    kBotCommand,    ///< For example, /start@jobs_bot
    kUrl,           ///< For example, https://telegram.org
    kEmail,         ///< For example, do-not-reply@telegram.org
    kPhoneNumber,   ///< For example, +1-212-555-0123
    kBold,          ///< For bold text
    kItalic,        ///< For italic text
    kUnderline,     ///< For underlined text
    kStrikethrough, ///< For strikethrough text
    kSpoiler,       ///< For spoiler message
    kCode,          ///< For monowidth string
    kPre,           ///< For monowidth block
    kTextLink,      ///< For clickable text URLs
    kTextMention,   ///< For users without usernames (https://telegram.org/blog/edit#new-mentions)
    kCustomEmoji    ///< For inline custom emoji stickers
  };

  /// @brief Type of the entity.
  Type type;

  /// @brief Offset in UTF-16 code units to the start of the entity.
  std::int64_t offset{};

  /// @brief Length of the entity in UTF-16 code units.
  std::int64_t length{};

  /// @brief Optional. URL that will be opened after user taps on the text.
  /// @note For Type::kTextLink only.
  std::optional<std::string> url;

  /// @brief Optional. The mentioned user.
  /// @note For Type::kTextMention only.
  std::unique_ptr<User> user;

  /// @brief Optional. The programming language of the entity text.
  /// @note For Type::kPre only.
  std::optional<std::string> language;

  /// @brief Optional. Unique identifier of the custom emoji.
  /// @note For Type::kCustomEmoji only.
  /// @note Use GetCustomEmojiStickers to get full information about
  /// the sticker.
  std::optional<std::string> custom_emoji_id;
};

std::string_view ToString(MessageEntity::Type message_entity_type);

MessageEntity::Type Parse(const formats::json::Value& value,
                          formats::parse::To<MessageEntity::Type>);

formats::json::Value Serialize(MessageEntity::Type message_entity_type,
                               formats::serialize::To<formats::json::Value>);

MessageEntity Parse(const formats::json::Value& json,
                    formats::parse::To<MessageEntity>);

formats::json::Value Serialize(const MessageEntity& message_entity,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
