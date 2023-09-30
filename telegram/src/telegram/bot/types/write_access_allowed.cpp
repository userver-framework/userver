#include <userver/telegram/bot/types/write_access_allowed.hpp>

#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
WriteAccessAllowed Parse(const Value& data,
                         formats::parse::To<WriteAccessAllowed>) {
  return WriteAccessAllowed{
    data["from_request"].template As<std::optional<bool>>(),
    data["web_app_name"].template As<std::optional<std::string>>(),
    data["from_attachment_menu"].template As<std::optional<bool>>()
  };
}

template <class Value>
Value Serialize(const WriteAccessAllowed& write_access_allowed,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  SetIfNotNull(builder, "from_request", write_access_allowed.from_request);
  SetIfNotNull(builder, "web_app_name", write_access_allowed.web_app_name);
  SetIfNotNull(builder, "from_attachment_menu", write_access_allowed.from_attachment_menu);
  return builder.ExtractValue();
}

}  // namespace impl

WriteAccessAllowed Parse(const formats::json::Value& json,
                         formats::parse::To<WriteAccessAllowed> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const WriteAccessAllowed& write_access_allowed,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(write_access_allowed, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
