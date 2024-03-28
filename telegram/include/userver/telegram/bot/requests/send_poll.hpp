#pragma once

#include <userver/telegram/bot/requests/request.hpp>
#include <userver/telegram/bot/types/chat_id.hpp>
#include <userver/telegram/bot/types/message_entity.hpp>
#include <userver/telegram/bot/types/message.hpp>
#include <userver/telegram/bot/types/reply_markup.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Use this method to send a native poll.
/// @see https://core.telegram.org/bots/api#sendpoll
struct SendPollMethod {
  static constexpr std::string_view kName = "sendPoll";

  static constexpr auto kHttpMethod = clients::http::HttpMethod::kPost;

  struct Parameters{
    Parameters(ChatId _chat_id,
               std::string _question,
               std::vector<std::string> _options);

    /// @brief Unique identifier for the target chat or username of the target
    /// channel (in the format @channelusername).
    ChatId chat_id;

    /// @brief Unique identifier for the target message thread (topic) of 
    /// the forum.
    /// @note For forum supergroups only.
    std::optional<std::int64_t> message_thread_id;

    /// @brief Poll question, 1-300 characters.
    std::string question;

    /// @brief List of answer options, 2-10 strings 1-100 characters each.
    std::vector<std::string> options;

    /// @brief True, if the poll needs to be anonymous.
    /// @note Defaults to True.
    std::optional<bool> is_anonymous;

    /// @brief Poll type.
    /// @note Defaults to Poll::Type::Regular.
    std::optional<Poll::Type> type;

    /// @brief True, if the poll allows multiple answers, ignored for polls in
    /// quiz mode.
    /// @note Defaults to False.
    std::optional<bool> allows_multiple_answers;

    /// @brief 0-based identifier of the correct answer option.
    /// @note Required for polls in quiz mode.
    std::optional<std::int64_t> correct_option_id;

    /// @brief Text that is shown when a user chooses an incorrect answer
    /// or taps on the lamp icon in a quiz-style poll, 0-200 characters with
    /// at most 2 line feeds after entities parsing.
    std::optional<std::string> explanation;

    /// @brief Mode for parsing entities in the explanation. 
    /// @see https://core.telegram.org/bots/api#formatting-options
    /// for more details.
    std::optional<std::string> explanation_parse_mode;

    /// @brief AList of special entities that appear in the poll explanation,
    /// which can be specified instead of parse_mode.
    std::optional<std::vector<MessageEntity>> explanation_entities;

    /// @brief Amount of time in seconds the poll will be active
    /// after creation.
    /// @note Possible values: 5s-600s
    /// @note Can't be used together with close_date.
    std::optional<std::chrono::seconds> open_period;

    /// @brief Point in time (Unix timestamp) when the poll will be
    /// automatically closed.
    /// @note Must be at least 5 and no more than 600 seconds in the future. 
    /// @note Can't be used together with open_period.
    std::optional<std::chrono::system_clock::time_point> close_date;

    /// @brief Pass True if the poll needs to be immediately closed.
    /// @note This can be useful for poll preview.
    std::optional<bool> is_closed;

    /// @brief Sends the message silently. Users will receive a notification
    /// with no sound.
    std::optional<bool> disable_notification;

    /// @brief Protects the contents of the sent message from forwarding
    /// and saving.
    std::optional<bool> protect_content;

    /// @brief If the message is a reply, ID of the original message.
    std::optional<std::int64_t> reply_to_message_id;

    /// @brief Pass True if the message should be sent even if the specified
    /// replied-to message is not found.
    std::optional<bool> allow_sending_without_reply;

    /// @brief Additional interface options.
    /// @see https://core.telegram.org/bots/features#inline-keyboards
    /// @see https://core.telegram.org/bots/features#keyboards
    std::optional<ReplyMarkup> reply_markup;
  };

  /// @brief On success, the sent Message is returned.
  using Reply = Message;

  static void FillRequestData(clients::http::Request& request,
                              const Parameters& parameters);

  static Reply ParseResponseData(clients::http::Response& response);
};

SendPollMethod::Parameters Parse(const formats::json::Value& json,
                                 formats::parse::To<SendPollMethod::Parameters>);

formats::json::Value Serialize(const SendPollMethod::Parameters& parameters,
                               formats::serialize::To<formats::json::Value>);

using SendPollRequest = Request<SendPollMethod>;

}  // namespace telegram::bot

USERVER_NAMESPACE_END
