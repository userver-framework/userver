#include <userver/telegram/bot/types/audio.hpp>

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
Audio Parse(const Value& data, formats::parse::To<Audio>) {
  return Audio{
    data["file_id"].template As<std::string>(),
    data["file_unique_id"].template As<std::string>(),
    data["duration"].template As<std::int64_t>(),
    data["performer"].template As<std::optional<std::string>>(),
    data["title"].template As<std::optional<std::string>>(),
    data["file_name"].template As<std::optional<std::string>>(),
    data["mime_type"].template As<std::optional<std::string>>(),
    data["file_size"].template As<std::optional<std::int64_t>>(),
    data["thumbnail"].template As<std::unique_ptr<PhotoSize>>()
  };
}

template <class Value>
Value Serialize(const Audio& audio, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["file_id"] = audio.file_id;
  builder["file_unique_id"] = audio.file_unique_id;
  builder["duration"] = audio.duration;
  SetIfNotNull(builder, "performer", audio.performer);
  SetIfNotNull(builder, "title", audio.title);
  SetIfNotNull(builder, "file_name", audio.file_name);
  SetIfNotNull(builder, "mime_type", audio.mime_type);
  SetIfNotNull(builder, "file_size", audio.file_size);
  SetIfNotNull(builder, "thumbnail", audio.thumbnail);
  return builder.ExtractValue();
}

}  // namespace impl

Audio Parse(const formats::json::Value& json,
            formats::parse::To<Audio> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const Audio& audio,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(audio, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
