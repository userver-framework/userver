#include <userver/telegram/bot/types/location.hpp>

#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
Location Parse(const Value& data, formats::parse::To<Location>) {
  return Location{
    data["longitude"].template As<double>(),
    data["latitude"].template As<double>(),
    data["horizontal_accuracy"].template As<std::optional<double>>(),
    data["live_period"].template As<std::optional<std::int64_t>>(),
    data["heading"].template As<std::optional<std::int64_t>>(),
    data["proximity_alert_radius"].template As<std::optional<std::int64_t>>()
  };
}

template <class Value>
Value Serialize(const Location& location, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["longitude"] = location.longitude;
  builder["latitude"] = location.latitude;
  SetIfNotNull(builder, "horizontal_accuracy", location.horizontal_accuracy);
  SetIfNotNull(builder, "live_period", location.live_period);
  SetIfNotNull(builder, "heading", location.heading);
  SetIfNotNull(builder, "proximity_alert_radius", location.proximity_alert_radius);
  return builder.ExtractValue();
}

}  // namespace impl

Location Parse(const formats::json::Value& json,
               formats::parse::To<Location> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const Location& location,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(location, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
