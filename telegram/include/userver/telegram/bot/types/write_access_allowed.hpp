#pragma once

#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a service message about a user allowing a bot
/// to write messages after adding it to the attachment menu, launching a
/// Web App from a link, or accepting an explicit request from a Web App sent
/// by the method requestWriteAccess.
/// @see https://core.telegram.org/bots/api#writeaccessallowed
struct WriteAccessAllowed {
  /// @brief Optional. True, if the access was granted after the user accepted
  /// an explicit request from a Web App sent by the method RequestWriteAccess
  std::optional<bool> from_request;

  /// @brief Optional. Name of the Web App, if the access was granted when the
  /// Web App was launched from a link
  std::optional<std::string> web_app_name;

  /// @brief Optional. True, if the access was granted when the bot was added
  /// to the attachment or side menu
  std::optional<bool> from_attachment_menu;
};

WriteAccessAllowed Parse(const formats::json::Value& json,
                         formats::parse::To<WriteAccessAllowed>);

formats::json::Value Serialize(const WriteAccessAllowed& write_access_allowed,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
