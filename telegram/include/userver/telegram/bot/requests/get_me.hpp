#pragma once

#include <userver/telegram/bot/requests/request.hpp>
#include <userver/telegram/bot/types/user.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief A simple method for testing your bot's authentication token.
struct GetMeMethod {
  static constexpr std::string_view kName = "getMe";

  static constexpr auto kHttpMethod = clients::http::HttpMethod::kGet;

  /// @brief Requires no parameters.
  struct Parameters{};

  /// @brief Returns basic information about the bot in form of a User object.
  using Reply = User;

  static void FillRequestData(clients::http::Request& request,
                              const Parameters& parameters);

  static Reply ParseResponseData(clients::http::Response& response);
};

GetMeMethod::Parameters Parse(const formats::json::Value& json,
                              formats::parse::To<GetMeMethod::Parameters>);

formats::json::Value Serialize(const GetMeMethod::Parameters& parameters,
                               formats::serialize::To<formats::json::Value>);

using GetMeRequest = Request<GetMeMethod>;

}  // namespace telegram::bot

USERVER_NAMESPACE_END
