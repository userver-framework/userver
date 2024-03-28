#include <userver/telegram/bot/types/proximity_alert_triggered.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
ProximityAlertTriggered Parse(const Value& data,
                              formats::parse::To<ProximityAlertTriggered>) {
  return ProximityAlertTriggered{
    data["traveler"].template As<std::unique_ptr<User>>(),
    data["watcher"].template As<std::unique_ptr<User>>(),
    data["distance"].template As<std::int64_t>()
  };
}

template <class Value>
Value Serialize(const ProximityAlertTriggered& proximity_alert_triggered,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["traveler"] = proximity_alert_triggered.traveler;
  builder["watcher"] = proximity_alert_triggered.watcher;
  builder["distance"] = proximity_alert_triggered.distance;
  return builder.ExtractValue();
}

}  // namespace impl

ProximityAlertTriggered Parse(const formats::json::Value& json,
                              formats::parse::To<ProximityAlertTriggered> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const ProximityAlertTriggered& proximity_alert_triggered,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(proximity_alert_triggered, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
