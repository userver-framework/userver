#include <userver/telegram/bot/types/video.hpp>

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
Video Parse(const Value& data, formats::parse::To<Video>) {
  return Video{
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
Value Serialize(const Video& video, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["file_id"] = video.file_id;
  builder["file_unique_id"] = video.file_unique_id;
  builder["width"] = video.width;
  builder["height"] = video.height;
  builder["duration"] = video.duration;
  SetIfNotNull(builder, "thumbnail", video.thumbnail);
  SetIfNotNull(builder, "file_name", video.file_name);
  SetIfNotNull(builder, "mime_type", video.mime_type);
  SetIfNotNull(builder, "file_size", video.file_size);
  return builder.ExtractValue();
}

}  // namespace impl

Video Parse(const formats::json::Value& json,
            formats::parse::To<Video> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const Video& video,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(video, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
