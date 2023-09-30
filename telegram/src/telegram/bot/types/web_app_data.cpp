#include <userver/telegram/bot/types/web_app_data.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
WebAppData Parse(const Value& data, formats::parse::To<WebAppData>) {
  return WebAppData{
    data["data"].template As<std::string>(),
    data["button_text"].template As<std::string>()
  };
}

template <class Value>
Value Serialize(const WebAppData& web_app_data, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["data"] = web_app_data.data;
  builder["button_text"] = web_app_data.button_text;
  return builder.ExtractValue();
}

}  // namespace impl

WebAppData Parse(const formats::json::Value& json,
                 formats::parse::To<WebAppData> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const WebAppData& web_app_data,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(web_app_data, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
