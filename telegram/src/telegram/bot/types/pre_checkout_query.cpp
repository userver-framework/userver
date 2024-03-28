#include <userver/telegram/bot/types/pre_checkout_query.hpp>

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
PreCheckoutQuery Parse(const Value& data,
                       formats::parse::To<PreCheckoutQuery>) {
  return PreCheckoutQuery{
    data["id"].template As<std::string>(),
    data["from"].template As<std::unique_ptr<User>>(),
    data["currency"].template As<std::string>(),
    data["total_amount"].template As<std::int64_t>(),
    data["invoice_payload"].template As<std::string>(),
    data["shipping_option_id"].template As<std::optional<std::string>>(),
    data["order_info"].template As<std::unique_ptr<OrderInfo>>()
  };
}

template <class Value>
Value Serialize(const PreCheckoutQuery& pre_checkout_query,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["id"] = pre_checkout_query.id;
  builder["from"] = pre_checkout_query.from;
  builder["currency"] = pre_checkout_query.currency;
  builder["total_amount"] = pre_checkout_query.total_amount;
  builder["invoice_payload"] = pre_checkout_query.invoice_payload;
  SetIfNotNull(builder, "shipping_option_id", pre_checkout_query.shipping_option_id);
  SetIfNotNull(builder, "order_info", pre_checkout_query.order_info);
  return builder.ExtractValue();
}

}  // namespace impl

PreCheckoutQuery Parse(const formats::json::Value& json,
                       formats::parse::To<PreCheckoutQuery> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const PreCheckoutQuery& pre_checkout_query,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(pre_checkout_query, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
