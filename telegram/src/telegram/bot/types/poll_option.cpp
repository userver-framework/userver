#include <userver/telegram/bot/types/poll_option.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
PollOption Parse(const Value& data, formats::parse::To<PollOption>) {
  return PollOption{
    data["text"].template As<std::string>(),
    data["voter_count"].template As<std::int64_t>()
  };
}

template <class Value>
Value Serialize(const PollOption& poll_option, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["text"] = poll_option.text;
  builder["voter_count"] = poll_option.voter_count;
  return builder.ExtractValue();
}

}  // namespace impl

PollOption Parse(const formats::json::Value& json,
                 formats::parse::To<PollOption> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const PollOption& poll_option,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(poll_option, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
