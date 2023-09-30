#include <userver/telegram/bot/types/poll.hpp>

#include <telegram/bot/formats/value_builder.hpp>
#include <telegram/bot/types/time.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize_duration.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/parse/common.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace {

constexpr utils::TrivialBiMap kPoolTypeMap([](auto selector) {
  return selector()
    .Case(Poll::Type::kRegular, "regular")
    .Case(Poll::Type::kQuiz, "quiz");
});

}  // namespace

namespace impl {

template <class Value>
Poll Parse(const Value& data, formats::parse::To<Poll>) {
  return Poll{
    data["id"].template As<std::string>(),
    data["question"].template As<std::string>(),
    data["options"].template As<std::vector<PollOption>>(),
    data["total_voter_count"].template As<std::int64_t>(),
    data["is_closed"].template As<bool>(),
    data["is_anonymous"].template As<bool>(),
    data["type"].template As<Poll::Type>(),
    data["allows_multiple_answers"].template As<bool>(),
    data["correct_option_id"].template As<std::optional<std::int64_t>>(),
    data["explanation"].template As<std::optional<std::string>>(),
    data["explanation_entities"].template As<std::optional<std::vector<MessageEntity>>>(),
    data["open_period"].template As<std::optional<std::chrono::seconds>>(),
    TransformToTimePoint(data["close_date"].template As<std::optional<std::chrono::seconds>>())
  };
}

template <class Value>
Value Serialize(const Poll& poll, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["id"] = poll.id;
  builder["question"] = poll.question;
  builder["options"] = poll.options;
  builder["total_voter_count"] = poll.total_voter_count;
  builder["is_closed"] = poll.is_closed;
  builder["is_anonymous"] = poll.is_anonymous;
  builder["type"] = poll.type;
  builder["allows_multiple_answers"] = poll.allows_multiple_answers;
  SetIfNotNull(builder, "correct_option_id", poll.correct_option_id);
  SetIfNotNull(builder, "explanation", poll.explanation);
  SetIfNotNull(builder, "explanation_entities", poll.explanation_entities);
  SetIfNotNull(builder, "open_period", poll.open_period);
  SetIfNotNull(builder, "close_date", TransformToSeconds(poll.close_date));
  return builder.ExtractValue();
}

}  // namespace impl

std::string_view ToString(Poll::Type poll_type) {
  return utils::impl::EnumToStringView(poll_type, kPoolTypeMap);
}

Poll::Type Parse(const formats::json::Value& value,
                 formats::parse::To<Poll::Type>) {
  return utils::ParseFromValueString(value, kPoolTypeMap);
}

formats::json::Value Serialize(Poll::Type poll_type,
                               formats::serialize::To<formats::json::Value>) {
  return formats::json::ValueBuilder(ToString(poll_type)).ExtractValue();
}

Poll Parse(const formats::json::Value& json,
           formats::parse::To<Poll> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const Poll& poll,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(poll, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
