#pragma once

#include <userver/telegram/bot/types/chat.hpp>
#include <userver/telegram/bot/types/user.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents an answer of a user in a non-anonymous poll.
/// @see https://core.telegram.org/bots/api#pollanswer
struct PollAnswer {
  /// @brief Unique poll identifier.
  std::string poll_id;

  /// @brief Optional. The chat that changed the answer to the poll,
  /// if the voter is anonymous.
  std::unique_ptr<Chat> voter_chat;

  /// @brief Optional. The user that changed the answer to the poll,
  /// if the voter isn't anonymous.
  std::unique_ptr<User> user;

  /// @brief 0-based identifiers of chosen answer options.
  /// @note May be empty if the vote was retracted.
  std::vector<std::int64_t> option_ids;
};

PollAnswer Parse(const formats::json::Value& json,
                 formats::parse::To<PollAnswer>);

formats::json::Value Serialize(const PollAnswer& poll_answer,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
