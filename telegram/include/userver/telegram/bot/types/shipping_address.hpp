#pragma once

#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a shipping address.
/// @see https://core.telegram.org/bots/api#shippingaddress
struct ShippingAddress {
  /// @brief Two-letter ISO 3166-1 alpha-2 country code.
  std::string country_code;

  /// @brief State, if applicable.
  std::string state;

  /// @brief City.
  std::string city;

  /// @brief First line for the address.
  std::string street_line1;

  /// @brief Second line for the address.
  std::string street_line2;

  /// @brief Address post code.
  std::string post_code;
};

ShippingAddress Parse(const formats::json::Value& json,
                      formats::parse::To<ShippingAddress>);

formats::json::Value Serialize(const ShippingAddress& shipping_address,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
