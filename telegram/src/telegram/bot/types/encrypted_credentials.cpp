#include <userver/telegram/bot/types/encrypted_credentials.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
EncryptedCredentials Parse(const Value& data,
                           formats::parse::To<EncryptedCredentials>) {
  return EncryptedCredentials{
    data["data"].template As<std::string>(),
    data["hash"].template As<std::string>(),
    data["secret"].template As<std::string>()
  };
}

template <class Value>
Value Serialize(const EncryptedCredentials& encrypted_credentials,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["data"] = encrypted_credentials.data;
  builder["hash"] = encrypted_credentials.hash;
  builder["secret"] = encrypted_credentials.secret;
  return builder.ExtractValue();
}

}  // namespace impl

EncryptedCredentials Parse(const formats::json::Value& json,
                           formats::parse::To<EncryptedCredentials> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const EncryptedCredentials& encrypted_credentials,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(encrypted_credentials, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
