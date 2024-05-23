#include <userver/telegram/bot/types/web_app_info.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
WebAppInfo Parse(const Value& data, formats::parse::To<WebAppInfo>) {
  return WebAppInfo{
    data["url"].template As<std::string>()
  };
}

template <class Value>
Value Serialize(const WebAppInfo& web_app_info, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["url"] = web_app_info.url;
  return builder.ExtractValue();
}

}  // namespace impl

WebAppInfo Parse(const formats::json::Value& json,
                 formats::parse::To<WebAppInfo> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const WebAppInfo& web_app_info,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(web_app_info, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
