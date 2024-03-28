#include <userver/telegram/bot/types/voice.hpp>

#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
Voice Parse(const Value& data, formats::parse::To<Voice>) {
  return Voice{
    data["file_id"].template As<std::string>(),
    data["file_unique_id"].template As<std::string>(),
    data["duration"].template As<std::int64_t>(),
    data["mime_type"].template As<std::optional<std::string>>(),
    data["file_size"].template As<std::optional<std::int64_t>>()
  };
}

template <class Value>
Value Serialize(const Voice& voice, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["file_id"] = voice.file_id;
  builder["file_unique_id"] = voice.file_unique_id;
  builder["duration"] = voice.duration;
  SetIfNotNull(builder, "mime_type", voice.mime_type);
  SetIfNotNull(builder, "file_size", voice.file_size);
  return builder.ExtractValue();
}

}  // namespace impl

Voice Parse(const formats::json::Value& json,
            formats::parse::To<Voice> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const Voice& voice,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(voice, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
