#include <userver/telegram/bot/types/user_shared.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
UserShared Parse(const Value& data, formats::parse::To<UserShared>) {
  return UserShared{
    data["request_id"].template As<std::int64_t>(),
    data["user_id"].template As<std::int64_t>()
  };
}

template <class Value>
Value Serialize(const UserShared& user_shared, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["request_id"] = user_shared.request_id;
  builder["user_id"] = user_shared.user_id;
  return builder.ExtractValue();
}

}  // namespace impl

UserShared Parse(const formats::json::Value& json,
                 formats::parse::To<UserShared> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const UserShared& user_shared,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(user_shared, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
