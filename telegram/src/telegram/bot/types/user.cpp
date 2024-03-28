#include <userver/telegram/bot/types/user.hpp>

#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
User Parse(const Value& data, formats::parse::To<User>) {
  return User{
    data["id"].template As<std::int64_t>(),
    data["is_bot"].template As<bool>(),
    data["first_name"].template As<std::string>(),
    data["last_name"].template As<std::optional<std::string>>(),
    data["username"].template As<std::optional<std::string>>(),
    data["language_code"].template As<std::optional<std::string>>(),
    data["is_premium"].template As<std::optional<bool>>(),
    data["added_to_attachment_menu"].template As<std::optional<bool>>(),
    data["can_join_groups"].template As<std::optional<bool>>(),
    data["can_read_all_group_messages"].template As<std::optional<bool>>(),
    data["supports_inline_queries"].template As<std::optional<bool>>()
  };
}

template <class Value>
Value Serialize(const User& user, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["id"] = user.id;
  builder["is_bot"] = user.is_bot;
  builder["first_name"] = user.first_name;
  SetIfNotNull(builder, "last_name", user.last_name);
  SetIfNotNull(builder, "username", user.username);
  SetIfNotNull(builder, "language_code", user.language_code);
  SetIfNotNull(builder, "is_premium", user.is_premium);
  SetIfNotNull(builder, "added_to_attachment_menu", user.added_to_attachment_menu);
  SetIfNotNull(builder, "can_join_groups", user.can_join_groups);
  SetIfNotNull(builder, "can_read_all_group_messages", user.can_read_all_group_messages);
  SetIfNotNull(builder, "supports_inline_queries", user.supports_inline_queries);
  return builder.ExtractValue();
}

}  // namespace impl

User Parse(const formats::json::Value& json,
           formats::parse::To<User> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const User& user,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(user, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
