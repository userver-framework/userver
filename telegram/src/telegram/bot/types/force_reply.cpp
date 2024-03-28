#include <userver/telegram/bot/types/force_reply.hpp>

#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
ForceReply Parse(const Value& data, formats::parse::To<ForceReply>) {
  return ForceReply{
    data["force_reply"].template As<bool>(),
    data["input_field_placeholder"].template As<std::optional<std::string>>(),
    data["selective"].template As<std::optional<bool>>()
  };
}

template <class Value>
Value Serialize(const ForceReply& force_reply, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["force_reply"] = force_reply.force_reply;
  SetIfNotNull(builder, "input_field_placeholder", force_reply.input_field_placeholder);
  SetIfNotNull(builder, "selective", force_reply.selective);
  return builder.ExtractValue();
}

}  // namespace impl

ForceReply Parse(const formats::json::Value& json,
                 formats::parse::To<ForceReply> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const ForceReply& force_reply,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(force_reply, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
