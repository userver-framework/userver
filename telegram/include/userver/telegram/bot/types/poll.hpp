#pragma once

#include <userver/telegram/bot/types/message_entity.hpp>
#include <userver/telegram/bot/types/poll_option.hpp>

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object contains information about a poll.
/// @see https://core.telegram.org/bots/api#poll
struct Poll {
  /// @brief Unique poll identifier.
  std::string id;

  /// @brief Poll question, 1-300 characters.
  std::string question;

  /// @brief List of poll options.
  std::vector<PollOption> options;

  /// @brief Total number of users that voted in the poll.
  std::int64_t total_voter_count{};

  /// @brief True, if the poll is closed.
  bool is_closed{};

  /// @brief True, if the poll is anonymous.
  bool is_anonymous{};

  enum class Type {
    kRegular, kQuiz
  };

  /// @brief Poll type.
  Type type;

  /// @brief True, if the poll allows multiple answers.
  bool allows_multiple_answers{};

  /// @brief Optional. 0-based identifier of the correct answer option.
  /// @note Available only for polls in the quiz mode, which are closed, 
  /// or was sent (not forwarded) by the bot or to the private chat with the bot.
  std::optional<std::int64_t> correct_option_id;

  /// @brief Optional. Text that is shown when a user chooses an incorrect
  /// answer or taps on the lamp icon in a quiz-style poll, 0-200 characters.
  std::optional<std::string> explanation;

  /// @brief Optional. Special entities like usernames, URLs,
  /// bot commands, etc. that appear in the explanation.
  std::optional<std::vector<MessageEntity>> explanation_entities;

  /// @brief Optional. Amount of time in seconds the poll
  /// will be active after creation.
  std::optional<std::chrono::seconds> open_period;

  /// @brief Optional. Point in time (Unix timestamp) when the poll
  /// will be automatically closed.
  std::optional<std::chrono::system_clock::time_point> close_date;
};

std::string_view ToString(Poll::Type poll_type);

Poll::Type Parse(const formats::json::Value& value,
                 formats::parse::To<Poll::Type>);

formats::json::Value Serialize(Poll::Type poll_type,
                               formats::serialize::To<formats::json::Value>);

Poll Parse(const formats::json::Value& json,
           formats::parse::To<Poll>);

formats::json::Value Serialize(const Poll& poll,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
