#include <userver/telegram/bot/types/shipping_address.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
ShippingAddress Parse(const Value& data, formats::parse::To<ShippingAddress>) {
  return ShippingAddress{
    data["country_code"].template As<std::string>(),
    data["state"].template As<std::string>(),
    data["city"].template As<std::string>(),
    data["street_line1"].template As<std::string>(),
    data["street_line2"].template As<std::string>(),
    data["post_code"].template As<std::string>()
  };
}

template <class Value>
Value Serialize(const ShippingAddress& shipping_address, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["country_code"] = shipping_address.country_code;
  builder["state"] = shipping_address.state;
  builder["city"] = shipping_address.city;
  builder["street_line1"] = shipping_address.street_line1;
  builder["street_line2"] = shipping_address.street_line2;
  builder["post_code"] = shipping_address.post_code;
  return builder.ExtractValue();
}

}  // namespace impl

ShippingAddress Parse(const formats::json::Value& json,
                      formats::parse::To<ShippingAddress> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const ShippingAddress& shipping_address,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(shipping_address, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
