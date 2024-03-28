#include <userver/telegram/bot/types/passport_data.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
PassportData Parse(const Value& data, formats::parse::To<PassportData>) {
  return PassportData{
    data["data"].template As<std::vector<EncryptedPassportElement>>(),
    data["credentials"].template As<std::unique_ptr<EncryptedCredentials>>()
  };
}

template <class Value>
Value Serialize(const PassportData& passport_data,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["data"] = passport_data.data;
  builder["credentials"] = passport_data.credentials;
  return builder.ExtractValue();
}

}  // namespace impl

PassportData Parse(const formats::json::Value& json,
                   formats::parse::To<PassportData> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const PassportData& passport_data,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(passport_data, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
