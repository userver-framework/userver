#include <userver/telegram/bot/types/login_url.hpp>

#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
LoginUrl Parse(const Value& data, formats::parse::To<LoginUrl>) {
  return LoginUrl{
    data["url"].template As<std::string>(),
    data["forward_text"].template As<std::optional<std::string>>(),
    data["bot_username"].template As<std::optional<std::string>>(),
    data["request_write_access"].template As<std::optional<bool>>()
  };
}

template <class Value>
Value Serialize(const LoginUrl& login_url, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["url"] = login_url.url;
  SetIfNotNull(builder, "forward_text", login_url.forward_text);
  SetIfNotNull(builder, "bot_username", login_url.bot_username);
  SetIfNotNull(builder, "request_write_access", login_url.request_write_access);
  return builder.ExtractValue();
}

}  // namespace impl

LoginUrl Parse(const formats::json::Value& json,
               formats::parse::To<LoginUrl> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const LoginUrl& login_url,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(login_url, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
