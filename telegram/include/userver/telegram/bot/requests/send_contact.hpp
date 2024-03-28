#pragma once

#include <userver/telegram/bot/requests/request.hpp>
#include <userver/telegram/bot/types/chat_id.hpp>
#include <userver/telegram/bot/types/message.hpp>
#include <userver/telegram/bot/types/reply_markup.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Use this method to send phone contacts.
/// @see https://core.telegram.org/bots/api#sendcontact
struct SendContactMethod {
  static constexpr std::string_view kName = "sendContact";

  static constexpr auto kHttpMethod = clients::http::HttpMethod::kPost;

  struct Parameters{
    Parameters(ChatId _chat_id,
               std::string _phone_number,
               std::string _first_name);

    /// @brief Unique identifier for the target chat or username of the target
    /// channel (in the format @channelusername).
    ChatId chat_id;

    /// @brief Unique identifier for the target message thread (topic) of
    /// the forum.
    /// @note For forum supergroups only.
    std::optional<std::int64_t> message_thread_id;

    /// @brief Contact's phone number.
    std::string phone_number;

    /// @brief Contact's first name.
    std::string first_name;

    /// @brief Contact's last name.
    std::optional<std::string> last_name;

    /// @brief Additional data about the contact in the form of a vCard, 
    /// 0-2048 bytes.
    /// @see https://en.wikipedia.org/wiki/VCard
    std::optional<std::string> vcard;

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

SendContactMethod::Parameters Parse(
    const formats::json::Value& json,
    formats::parse::To<SendContactMethod::Parameters>);

formats::json::Value Serialize(const SendContactMethod::Parameters& parameters,
                               formats::serialize::To<formats::json::Value>);

using SendContactRequest = Request<SendContactMethod>;

}  // namespace telegram::bot

USERVER_NAMESPACE_END
