#include <userver/telegram/bot/types/animation.hpp>

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
Animation Parse(const Value& data, formats::parse::To<Animation>) {
  return Animation{
    data["file_id"].template As<std::string>(),
    data["file_unique_id"].template As<std::string>(),
    data["width"].template As<std::int64_t>(),
    data["height"].template As<std::int64_t>(),
    data["duration"].template As<std::int64_t>(),
    data["thumbnail"].template As<std::unique_ptr<PhotoSize>>(),
    data["file_name"].template As<std::optional<std::string>>(),
    data["mime_type"].template As<std::optional<std::string>>(),
    data["file_size"].template As<std::optional<std::int64_t>>()
  };
}

template <class Value>
Value Serialize(const Animation& animation, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["file_id"] = animation.file_id;
  builder["file_unique_id"] = animation.file_unique_id;
  builder["width"] = animation.width;
  builder["height"] = animation.height;
  builder["duration"] = animation.duration;
  SetIfNotNull(builder, "thumbnail", animation.thumbnail);
  SetIfNotNull(builder, "file_name", animation.file_name);
  SetIfNotNull(builder, "mime_type", animation.mime_type);
  SetIfNotNull(builder, "file_size", animation.file_size);
  return builder.ExtractValue();
}

}  // namespace impl

Animation Parse(const formats::json::Value& json,
                formats::parse::To<Animation> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const Animation& animation,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(animation, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
