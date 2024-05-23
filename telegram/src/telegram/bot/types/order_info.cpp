#include <userver/telegram/bot/types/order_info.hpp>

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
OrderInfo Parse(const Value& data, formats::parse::To<OrderInfo>) {
  return OrderInfo{
    data["name"].template As<std::optional<std::string>>(),
    data["phone_number"].template As<std::optional<std::string>>(),
    data["email"].template As<std::optional<std::string>>(),
    data["shipping_address"].template As<std::unique_ptr<ShippingAddress>>()
  };
}

template <class Value>
Value Serialize(const OrderInfo& order_info, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  SetIfNotNull(builder, "name", order_info.name);
  SetIfNotNull(builder, "phone_number", order_info.phone_number);
  SetIfNotNull(builder, "email", order_info.email);
  SetIfNotNull(builder, "shipping_address", order_info.shipping_address);
  return builder.ExtractValue();
}

}  // namespace impl

OrderInfo Parse(const formats::json::Value& json,
                formats::parse::To<OrderInfo> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const OrderInfo& order_info,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(order_info, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
