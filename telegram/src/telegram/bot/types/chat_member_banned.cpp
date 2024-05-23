#include <userver/telegram/bot/types/chat_member_banned.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>
#include <telegram/bot/types/time.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize_duration.hpp>
#include <userver/formats/parse/common.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
ChatMemberBanned Parse(const Value& data,
                       formats::parse::To<ChatMemberBanned>) {
  std::string status = data["status"].template As<std::string>();
  UINVARIANT(status == ChatMemberBanned::kStatus,
             fmt::format("Invalid status for ChatMemberBanned: {}, expected: {}",
                         status, ChatMemberBanned::kStatus));

  return ChatMemberBanned{
    data["user"].template As<std::unique_ptr<User>>(),
    TransformToTimePoint(data["until_date"].template As<std::chrono::seconds>())
  };
}

template <class Value>
Value Serialize(const ChatMemberBanned& chat_member_banned,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["status"] = ChatMemberBanned::kStatus;
  builder["user"] = chat_member_banned.user;
  builder["until_date"] = TransformToSeconds(chat_member_banned.until_date);
  return builder.ExtractValue();
}

}  // namespace impl

ChatMemberBanned Parse(const formats::json::Value& json,
                       formats::parse::To<ChatMemberBanned> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const ChatMemberBanned& chat_member_banned,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(chat_member_banned, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
