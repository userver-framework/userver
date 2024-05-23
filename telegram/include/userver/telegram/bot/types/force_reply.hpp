#pragma once

#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Upon receiving a message with this object, Telegram clients will
/// display a reply interface to the user (act as if the user has selected
/// the bot's message and tapped 'Reply').
/// @note This can be extremely useful if you want to create user-friendly
/// step-by-step interfaces without having to sacrifice privacy mode.
/// @example A poll bot for groups runs in privacy mode (only receives commands,
/// replies to its messages and mentions). There could be two ways to create a
/// new poll:
/// - Explain the user how to send a command with parameters (e.g. /newpoll
/// question answer1 answer2). May be appealing for hardcore users but lacks
/// modern day polish.
/// - Guide the user through a step-by-step process. 'Please send me your
/// question', 'Cool, now let's add the first answer option', 'Great. Keep
/// adding answer options, then send /done when you're ready'.
/// The last option is definitely more attractive. And if you use ForceReply
/// in your bot's questions, it will receive the user's answers even if it only
/// receives replies, commands and mentions - without any extra work for
/// the user.
/// @see https://core.telegram.org/bots/api#forcereply
struct ForceReply {
  /// @brief Shows reply interface to the user, as if they manually selected
  /// the bot's message and tapped 'Reply'.
  bool force_reply{};

  /// @brief Optional. The placeholder to be shown in the input field when
  /// the reply is active; 1-64 characters.
  std::optional<std::string> input_field_placeholder;

  /// @brief Optional. Use this parameter if you want to force reply from
  /// specific users only. Targets:
  /// 1) users that are @mentioned in the text of the Message object.
  /// 2) if the bot's message is a reply (has reply_to_message_id),
  /// sender of the original message.
  std::optional<bool> selective;
};

ForceReply Parse(const formats::json::Value& json,
                 formats::parse::To<ForceReply>);

formats::json::Value Serialize(const ForceReply& force_reply,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
