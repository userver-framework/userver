#include <userver/telegram/bot/types/photo_size.hpp>

#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
PhotoSize Parse(const Value& data, formats::parse::To<PhotoSize>) {
  return PhotoSize{
    data["file_id"].template As<std::string>(),
    data["file_unique_id"].template As<std::string>(),
    data["width"].template As<std::int64_t>(),
    data["height"].template As<std::int64_t>(),
    data["file_size"].template As<std::optional<std::int64_t>>()
  };
}

template <class Value>
Value Serialize(const PhotoSize& photo_size, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["file_id"] = photo_size.file_id;
  builder["file_unique_id"] = photo_size.file_unique_id;
  builder["width"] = photo_size.width;
  builder["height"] = photo_size.height;
  SetIfNotNull(builder, "file_size", photo_size.file_size);
  return builder.ExtractValue();
}

}  // namespace impl

PhotoSize Parse(const formats::json::Value& json,
                formats::parse::To<PhotoSize> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const PhotoSize& photo_size,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(photo_size, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
