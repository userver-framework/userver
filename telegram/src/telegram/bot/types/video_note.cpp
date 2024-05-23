#include <userver/telegram/bot/types/video_note.hpp>

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
VideoNote Parse(const Value& data, formats::parse::To<VideoNote>) {
  return VideoNote{
    data["file_id"].template As<std::string>(),
    data["file_unique_id"].template As<std::string>(),
    data["length"].template As<std::int64_t>(),
    data["duration"].template As<std::int64_t>(),
    data["thumbnail"].template As<std::unique_ptr<PhotoSize>>(),
    data["file_size"].template As<std::optional<std::int64_t>>()
  };
}

template <class Value>
Value Serialize(const VideoNote& video_note, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["file_id"] = video_note.file_id;
  builder["file_unique_id"] = video_note.file_unique_id;
  builder["length"] = video_note.length;
  builder["duration"] = video_note.duration;
  SetIfNotNull(builder, "thumbnail", video_note.thumbnail);
  SetIfNotNull(builder, "file_size", video_note.file_size);
  return builder.ExtractValue();
}

}  // namespace impl

VideoNote Parse(const formats::json::Value& json,
                formats::parse::To<VideoNote> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const VideoNote& video_note,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(video_note, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
