#pragma once

#include <userver/telegram/bot/types/shipping_address.hpp>
#include <userver/telegram/bot/types/user.hpp>

#include <memory>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object contains information about an incoming shipping query.
/// @see https://core.telegram.org/bots/api#shippingquery
struct ShippingQuery {
  /// @brief Unique query identifier.
  std::string id;

  /// @brief User who sent the query.
  std::unique_ptr<User> from;

  /// @brief Bot specified invoice payload.
  std::string invoice_payload;

  /// @brief User specified shipping address.
  std::unique_ptr<ShippingAddress> shipping_address;
};

ShippingQuery Parse(const formats::json::Value& json,
                    formats::parse::To<ShippingQuery>);

formats::json::Value Serialize(const ShippingQuery& shipping_query,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
