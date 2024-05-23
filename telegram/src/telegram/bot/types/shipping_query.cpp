#include <userver/telegram/bot/types/shipping_query.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
ShippingQuery Parse(const Value& data, formats::parse::To<ShippingQuery>) {
  return ShippingQuery{
    data["id"].template As<std::string>(),
    data["from"].template As<std::unique_ptr<User>>(),
    data["invoice_payload"].template As<std::string>(),
    data["shipping_address"].template As<std::unique_ptr<ShippingAddress>>()
  };
}

template <class Value>
Value Serialize(const ShippingQuery& shipping_query,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["id"] = shipping_query.id;
  builder["from"] = shipping_query.from;
  builder["invoice_payload"] = shipping_query.invoice_payload;
  builder["shipping_address"] = shipping_query.shipping_address;
  return builder.ExtractValue();
}

}  // namespace impl

ShippingQuery Parse(const formats::json::Value& json,
                    formats::parse::To<ShippingQuery> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const ShippingQuery& shipping_query,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(shipping_query, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
