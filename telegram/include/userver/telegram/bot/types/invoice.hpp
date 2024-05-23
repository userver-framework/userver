#pragma once

#include <cstdint>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object contains basic information about an invoice.
/// @see https://core.telegram.org/bots/api#invoice
struct Invoice {
  /// @brief Product name.
  std::string title;

  /// @brief Product description.
  std::string description;

  /// @brief Unique bot deep-linking parameter that can be used to
  /// generate this invoice.
  std::string start_parameter;

  /// @brief Three-letter ISO 4217 currency code.
  std::string currency;

  /// @brief Total price in the smallest units of the currency
  /// (integer, not float/double). For example, for a price of US$ 1.45
  /// pass amount = 145.
  /// @see the exp parameter in https://core.telegram.org/bots/payments/currencies.json,
  /// it shows the number of digits past the decimal point for each currency
  /// (2 for the majority of currencies).
  std::int64_t total_amount{};
};

Invoice Parse(const formats::json::Value& json,
              formats::parse::To<Invoice>);

formats::json::Value Serialize(const Invoice& invoice,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
