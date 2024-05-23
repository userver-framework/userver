#pragma once

#include <userver/telegram/bot/types/order_info.hpp>
#include <userver/telegram/bot/types/user.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object contains information about an incoming
/// pre-checkout query.
/// @see https://core.telegram.org/bots/api#precheckoutquery
struct PreCheckoutQuery {
  /// @brief Unique query identifier.
  std::string id;

  /// @brief User who sent the query.
  std::unique_ptr<User> from;

  /// @brief Three-letter ISO 4217 currency code.
  std::string currency;

  /// @brief Total price in the smallest units of the currency
  /// (integer, not float/double). For example, for a price of US$ 1.45
  /// pass amount = 145.
  /// @see the exp parameter in https://core.telegram.org/bots/payments/currencies.json,
  /// it shows the number of digits past the decimal point for each currency
  /// (2 for the majority of currencies).
  std::int64_t total_amount{};

  /// @brief Bot specified invoice payload.
  std::string invoice_payload;

  /// @brief Optional. Identifier of the shipping option chosen by the user.
  std::optional<std::string> shipping_option_id;

  /// @brief Optional. Order information provided by the user.
  std::unique_ptr<OrderInfo> order_info;
};

PreCheckoutQuery Parse(const formats::json::Value& json,
                       formats::parse::To<PreCheckoutQuery>);

formats::json::Value Serialize(const PreCheckoutQuery& pre_checkout_query,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
