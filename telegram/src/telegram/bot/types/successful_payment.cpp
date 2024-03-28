#include <userver/telegram/bot/types/successful_payment.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>
#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
SuccessfulPayment Parse(const Value& data,
                        formats::parse::To<SuccessfulPayment>) {
  return SuccessfulPayment{
    data["currency"].template As<std::string>(),
    data["total_amount"].template As<std::int64_t>(),
    data["invoice_payload"].template As<std::string>(),
    data["shipping_option_id"].template As<std::optional<std::string>>(),
    data["order_info"].template As<std::unique_ptr<OrderInfo>>(),
    data["telegram_payment_charge_id"].template As<std::string>(),
    data["provider_payment_charge_id"].template As<std::string>()
  };
}

template <class Value>
Value Serialize(const SuccessfulPayment& successful_payment,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["currency"] = successful_payment.currency;
  builder["total_amount"] = successful_payment.total_amount;
  builder["invoice_payload"] = successful_payment.invoice_payload;
  SetIfNotNull(builder, "shipping_option_id", successful_payment.shipping_option_id);
  SetIfNotNull(builder, "order_info", successful_payment.order_info);
  builder["telegram_payment_charge_id"] = successful_payment.telegram_payment_charge_id;
  builder["provider_payment_charge_id"] = successful_payment.provider_payment_charge_id;
  return builder.ExtractValue();
}

}  // namespace impl

SuccessfulPayment Parse(const formats::json::Value& json,
                        formats::parse::To<SuccessfulPayment> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const SuccessfulPayment& successful_payment,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(successful_payment, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
