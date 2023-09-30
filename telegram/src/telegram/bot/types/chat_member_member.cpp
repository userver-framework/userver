#include <userver/telegram/bot/types/chat_member_member.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>

#include <userver/formats/json.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
ChatMemberMember Parse(const Value& data,
                       formats::parse::To<ChatMemberMember>) {
  std::string status = data["status"].template As<std::string>();
  UINVARIANT(status == ChatMemberMember::kStatus,
             fmt::format("Invalid status for ChatMemberMember: {}, expected: {}",
                         status, ChatMemberMember::kStatus));

  return ChatMemberMember{
    data["user"].template As<std::unique_ptr<User>>()
  };
}

template <class Value>
Value Serialize(const ChatMemberMember& chat_member_member,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["status"] = ChatMemberMember::kStatus;
  builder["user"] = chat_member_member.user;
  return builder.ExtractValue();
}

}  // namespace impl

ChatMemberMember Parse(const formats::json::Value& json,
                       formats::parse::To<ChatMemberMember> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const ChatMemberMember& chat_member_member,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(chat_member_member, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
