#include <userver/telegram/bot/types/chat_member_left.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>

#include <userver/formats/json.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
ChatMemberLeft Parse(const Value& data, formats::parse::To<ChatMemberLeft>) {
  std::string status = data["status"].template As<std::string>();
  UINVARIANT(status == ChatMemberLeft::kStatus,
             fmt::format("Invalid status for ChatMemberLeft: {}, expected: {}",
                         status, ChatMemberLeft::kStatus));

  return ChatMemberLeft{
    data["user"].template As<std::unique_ptr<User>>()
  };
}

template <class Value>
Value Serialize(const ChatMemberLeft& chat_member_left,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["status"] = ChatMemberLeft::kStatus;
  builder["user"] = chat_member_left.user;
  return builder.ExtractValue();
}

}  // namespace impl

ChatMemberLeft Parse(const formats::json::Value& json,
                     formats::parse::To<ChatMemberLeft> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const ChatMemberLeft& chat_member_left,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(chat_member_left, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
