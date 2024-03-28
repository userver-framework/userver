#pragma once

#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Describes a Web App.
/// @see https://core.telegram.org/bots/api#webappinfo
struct WebAppInfo {
  /// @brief An HTTPS URL of a Web App to be opened with additional data as
  /// specified in Initializing Web Apps
  std::string url;
};

WebAppInfo Parse(const formats::json::Value& json,
                 formats::parse::To<WebAppInfo>);

formats::json::Value Serialize(const WebAppInfo& web_app_info,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
