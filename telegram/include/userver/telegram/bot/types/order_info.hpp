#pragma once

#include <userver/telegram/bot/types/shipping_address.hpp>

#include <memory>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents information about an order.
/// @see https://core.telegram.org/bots/api#orderinfo
struct OrderInfo {
  /// @brief Optional. User name.
  std::optional<std::string> name;

  /// @brief Optional. User's phone number.
  std::optional<std::string> phone_number;

  /// @brief Optional. User email.
  std::optional<std::string> email;

  /// @brief Optional. User shipping address.
  std::unique_ptr<ShippingAddress> shipping_address;
};

OrderInfo Parse(const formats::json::Value& json,
                formats::parse::To<OrderInfo>);

formats::json::Value Serialize(const OrderInfo& order_info,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
