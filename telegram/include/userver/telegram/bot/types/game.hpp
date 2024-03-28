#pragma once

#include <userver/telegram/bot/types/animation.hpp>
#include <userver/telegram/bot/types/message_entity.hpp>
#include <userver/telegram/bot/types/photo_size.hpp>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a game.
/// @note Use BotFather to create and edit games, their short
/// names will act as unique identifiers.
/// @see https://core.telegram.org/bots/api#game
struct Game {
  /// @brief Title of the game
  std::string title;

  /// @brief Description of the game
  std::string description;

  /// @brief Photo that will be displayed in the game message in chats.
  std::vector<PhotoSize> photo;

  /// @brief Optional. Brief description of the game or high scores included
  /// in the game message. Can be automatically edited to include current high
  /// scores for the game when the bot calls SetGameScore, or manually edited
  /// using EditMessageText. 0-4096 characters.
  std::optional<std::string> text;

  /// @brief Optional. Special entities that appear in text,
  /// such as usernames, URLs, bot commands, etc.
  std::optional<std::vector<MessageEntity>> text_entities;

  /// @brief Optional. Animation that will be displayed
  /// in the game message in chats.
  /// @note Upload via BotFather
  std::unique_ptr<Animation> animation;
};

Game Parse(const formats::json::Value& json,
           formats::parse::To<Game>);

formats::json::Value Serialize(const Game& game,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
