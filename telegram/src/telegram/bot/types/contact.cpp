#include <userver/telegram/bot/types/contact.hpp>

#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
Contact Parse(const Value& data, formats::parse::To<Contact>) {
  return Contact{
    data["phone_number"].template As<std::string>(),
    data["first_name"].template As<std::string>(),
    data["last_name"].template As<std::optional<std::string>>(),
    data["user_id"].template As<std::optional<std::int64_t>>(),
    data["vcard"].template As<std::optional<std::string>>()
  };
}

template <class Value>
Value Serialize(const Contact& contact, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["phone_number"] = contact.phone_number;
  builder["first_name"] = contact.first_name;
  SetIfNotNull(builder, "last_name", contact.last_name);
  SetIfNotNull(builder, "user_id", contact.user_id);
  SetIfNotNull(builder, "vcard", contact.vcard);
  return builder.ExtractValue();
}

}  // namespace impl

Contact Parse(const formats::json::Value& json,
              formats::parse::To<Contact> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const Contact& contact,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(contact, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
