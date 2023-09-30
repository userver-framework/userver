#include <userver/telegram/bot/types/chat_invite_link.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>
#include <telegram/bot/formats/value_builder.hpp>
#include <telegram/bot/types/time.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize_duration.hpp>
#include <userver/formats/parse/common.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
ChatInviteLink Parse(const Value& data, formats::parse::To<ChatInviteLink>) {
  return ChatInviteLink{
    data["invite_link"].template As<std::string>(),
    data["creator"].template As<std::unique_ptr<User>>(),
    data["creates_join_request"].template As<bool>(),
    data["is_primary"].template As<bool>(),
    data["is_revoked"].template As<bool>(),
    data["name"].template As<std::optional<std::string>>(),
    TransformToTimePoint(data["expire_date"].template As<std::optional<std::chrono::seconds>>()),
    data["member_limit"].template As<std::optional<std::int64_t>>(),
    data["pending_join_request_count"].template As<std::optional<std::int64_t>>()
  };
}

template <class Value>
Value Serialize(const ChatInviteLink& chat_invite_link,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["invite_link"] = chat_invite_link.invite_link;
  builder["creator"] = chat_invite_link.creator;
  builder["creates_join_request"] = chat_invite_link.creates_join_request;
  builder["is_primary"] = chat_invite_link.is_primary;
  builder["is_revoked"] = chat_invite_link.is_revoked;
  SetIfNotNull(builder, "name", chat_invite_link.name);
  SetIfNotNull(builder, "expire_date", TransformToSeconds(chat_invite_link.expire_date));
  SetIfNotNull(builder, "member_limit", chat_invite_link.member_limit);
  builder["pending_join_request_count"] = chat_invite_link.pending_join_request_count;
  return builder.ExtractValue();
}

}  // namespace impl

ChatInviteLink Parse(const formats::json::Value& json,
                     formats::parse::To<ChatInviteLink> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const ChatInviteLink& chat_invite_link,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(chat_invite_link, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
