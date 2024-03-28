#include <userver/telegram/bot/types/poll_answer.hpp>

#include <userver/telegram/bot/types/message.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>
#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
PollAnswer Parse(const Value& data, formats::parse::To<PollAnswer>) {
  return PollAnswer{
    data["poll_id"].template As<std::string>(),
    data["voter_chat"].template As<std::unique_ptr<Chat>>(),
    data["user"].template As<std::unique_ptr<User>>(),
    data["option_ids"].template As<std::vector<std::int64_t>>()
  };
}

template <class Value>
Value Serialize(const PollAnswer& poll_answer, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["poll_id"] = poll_answer.poll_id;
  SetIfNotNull(builder, "voter_chat", poll_answer.voter_chat);
  SetIfNotNull(builder, "user", poll_answer.user);
  builder["option_ids"] = poll_answer.option_ids;
  return builder.ExtractValue();
}

}  // namespace impl

PollAnswer Parse(const formats::json::Value& json,
                 formats::parse::To<PollAnswer> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const PollAnswer& poll_answer,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(poll_answer, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
