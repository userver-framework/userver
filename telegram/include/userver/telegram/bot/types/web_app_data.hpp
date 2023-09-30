#pragma once

#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Describes data sent from a Web App to the bot.
/// @see https://core.telegram.org/bots/api#webappdata
/// @see https://core.telegram.org/bots/webapps
struct WebAppData {
  /// @brief The data.
  /// @note Be aware that a bad client can send arbitrary data in this field.
  std::string data;

  /// @brief Text of the web_app keyboard button from which the Web App 
  /// was opened.
  /// @note Be aware that a bad client can send arbitrary data in this field.
  std::string button_text;
};

WebAppData Parse(const formats::json::Value& json,
                 formats::parse::To<WebAppData>);

formats::json::Value Serialize(const WebAppData& web_app_data,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
