#include <userver/telegram/bot/types/venue.hpp>

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
Venue Parse(const Value& data, formats::parse::To<Venue>) {
  return Venue{
    data["location"].template As<std::unique_ptr<Location>>(),
    data["title"].template As<std::string>(),
    data["address"].template As<std::string>(),
    data["foursquare_id"].template As<std::optional<std::string>>(),
    data["foursquare_type"].template As<std::optional<std::string>>(),
    data["google_place_id"].template As<std::optional<std::string>>(),
    data["google_place_type"].template As<std::optional<std::string>>()
  };
}

template <class Value>
Value Serialize(const Venue& venue, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["location"] = venue.location;
  builder["title"] = venue.title;
  builder["address"] = venue.address;
  SetIfNotNull(builder, "foursquare_id", venue.foursquare_id);
  SetIfNotNull(builder, "foursquare_type", venue.foursquare_type);
  SetIfNotNull(builder, "google_place_id", venue.google_place_id);
  SetIfNotNull(builder, "google_place_type", venue.google_place_type);
  return builder.ExtractValue();
}

}  // namespace impl

Venue Parse(const formats::json::Value& json,
            formats::parse::To<Venue> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const Venue& venue,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(venue, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
