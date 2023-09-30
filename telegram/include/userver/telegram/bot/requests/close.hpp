#pragma once

#include <userver/telegram/bot/requests/request.hpp>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Use this method to close the bot instance before moving it from one
/// local server to another.
/// @note You need to delete the webhook before calling this method to ensure
/// that the bot isn't launched again after server restart.
/// @note The method will return error 429 in the first 10 minutes after the
/// bot is launched.
/// @see https://core.telegram.org/bots/api#close
struct CloseMethod {
  static constexpr std::string_view kName = "close";

  static constexpr auto kHttpMethod = clients::http::HttpMethod::kPost;

  /// @brief Requires no parameters.
  struct Parameters{};

  /// @brief Returns True on success.
  using Reply = bool;

  static void FillRequestData(clients::http::Request& request,
                              const Parameters& parameters);

  static Reply ParseResponseData(clients::http::Response& response);
};

CloseMethod::Parameters Parse(const formats::json::Value& json,
                              formats::parse::To<CloseMethod::Parameters>);

formats::json::Value Serialize(const CloseMethod::Parameters& parameters,
                               formats::serialize::To<formats::json::Value>);

using CloseRequest = Request<CloseMethod>;

}  // namespace telegram::bot

USERVER_NAMESPACE_END
