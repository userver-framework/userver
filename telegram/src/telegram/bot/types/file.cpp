#include <userver/telegram/bot/types/file.hpp>

#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
File Parse(const Value& data, formats::parse::To<File>) {
  return File{
    data["file_id"].template As<std::string>(),
    data["file_unique_id"].template As<std::string>(),
    data["file_size"].template As<std::optional<std::int64_t>>(),
    data["file_path"].template As<std::optional<std::string>>()
  };
}

template <class Value>
Value Serialize(const File& file, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["file_id"] = file.file_id;
  builder["file_unique_id"] = file.file_unique_id;
  SetIfNotNull(builder, "file_size", file.file_size);
  SetIfNotNull(builder, "file_path", file.file_path);
  return builder.ExtractValue();
}

}  // namespace impl

File Parse(const formats::json::Value& json,
           formats::parse::To<File> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const File& file,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(file, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
