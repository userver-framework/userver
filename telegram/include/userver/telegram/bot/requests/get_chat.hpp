#pragma once

#include <userver/telegram/bot/requests/request.hpp>
#include <userver/telegram/bot/types/chat_id.hpp>
#include <userver/telegram/bot/types/chat.hpp>
#include <userver/telegram/bot/types/message.hpp>

#include <memory>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Use this method to get up to date information about the chat
/// (current name of the user for one-on-one conversations, current username
/// of a user, group or channel, etc.).
/// @see https://core.telegram.org/bots/api#getchat
struct GetChatMethod {
  static constexpr std::string_view kName = "getChat";

  static constexpr auto kHttpMethod = clients::http::HttpMethod::kGet;

  struct Parameters{
    Parameters(ChatId _chat_id);

    /// @brief Unique identifier for the target chat or username of the
    /// target supergroup or channel (in the format @channelusername).
    ChatId chat_id;
  };

  /// @brief Returns a Chat object on success.
  using Reply = Chat;

  static void FillRequestData(clients::http::Request& request,
                              const Parameters& parameters);

  static Reply ParseResponseData(clients::http::Response& response);
};

GetChatMethod::Parameters Parse(const formats::json::Value& json,
                                formats::parse::To<GetChatMethod::Parameters>);

formats::json::Value Serialize(const GetChatMethod::Parameters& parameters,
                               formats::serialize::To<formats::json::Value>);

using GetChatRequest = Request<GetChatMethod>;

}  // namespace telegram::bot

USERVER_NAMESPACE_END
