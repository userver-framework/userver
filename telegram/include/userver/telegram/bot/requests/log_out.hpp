#pragma once

#include <userver/telegram/bot/requests/request.hpp>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Use this method to log out from the cloud Bot API server before
/// launching the bot locally.
/// @note You must log out the bot before running it locally, otherwise there
/// is no guarantee that the bot will receive updates.
/// @note After a successful call, you can immediately log in on a local server,
/// but will not be able to log in back to the cloud Bot API server for 
/// 10 minutes.
/// @see https://core.telegram.org/bots/api#logout
struct LogOutMethod {
  static constexpr std::string_view kName = "logOut";

  static constexpr auto kHttpMethod = clients::http::HttpMethod::kPost;

  /// @brief Requires no parameters.
  struct Parameters{};

  /// @brief Returns True on success.
  using Reply = bool;

  static void FillRequestData(clients::http::Request& request,
                              const Parameters& parameters);

  static Reply ParseResponseData(clients::http::Response& response);
};

LogOutMethod::Parameters Parse(const formats::json::Value& json,
                               formats::parse::To<LogOutMethod::Parameters>);

formats::json::Value Serialize(const LogOutMethod::Parameters& parameters,
                               formats::serialize::To<formats::json::Value>);

using LogOutRequest = Request<LogOutMethod>;

}  // namespace telegram::bot

USERVER_NAMESPACE_END
