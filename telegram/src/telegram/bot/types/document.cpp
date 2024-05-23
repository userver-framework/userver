#include <userver/telegram/bot/types/document.hpp>

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
Document Parse(const Value& data, formats::parse::To<Document>) {
  return Document{
    data["file_id"].template As<std::string>(),
    data["file_unique_id"].template As<std::string>(),
    data["thumbnail"].template As<std::unique_ptr<PhotoSize>>(),
    data["file_name"].template As<std::optional<std::string>>(),
    data["mime_type"].template As<std::optional<std::string>>(),
    data["file_size"].template As<std::optional<std::int64_t>>()
  };
}

template <class Value>
Value Serialize(const Document& document, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["file_id"] = document.file_id;
  builder["file_unique_id"] = document.file_unique_id;
  SetIfNotNull(builder, "thumbnail", document.thumbnail);
  SetIfNotNull(builder, "file_name", document.file_name);
  SetIfNotNull(builder, "mime_type", document.mime_type);
  SetIfNotNull(builder, "file_size", document.file_size);
  return builder.ExtractValue();
}

}  // namespace impl

Document Parse(const formats::json::Value& json,
               formats::parse::To<Document> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const Document& document,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(document, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
