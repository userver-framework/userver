#pragma once

#include <userver/telegram/bot/types/user.hpp>

#include <cstdint>
#include <memory>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents the content of a service message, sent
/// whenever a user in the chat triggers a proximity alert set by another user.
/// @see https://core.telegram.org/bots/api#proximityalerttriggered
struct ProximityAlertTriggered {
  /// @brief User that triggered the alert
  std::unique_ptr<User> traveler;

  /// @brief User that set the alert
  std::unique_ptr<User> watcher;

  /// @brief The distance between the users
  std::int64_t distance{};
};

ProximityAlertTriggered Parse(const formats::json::Value& json,
                              formats::parse::To<ProximityAlertTriggered>);

formats::json::Value Serialize(const ProximityAlertTriggered& proximity_alert_triggered,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
