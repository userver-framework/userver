#include <userver/telegram/bot/types/message_entity.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>
#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace {

constexpr utils::TrivialBiMap kMessageEntityTypeMap([](auto selector) {
  return selector()
    .Case(MessageEntity::Type::kMention, "mention")
    .Case(MessageEntity::Type::kHashtag, "hashtag")
    .Case(MessageEntity::Type::kCashtag, "cashtag")
    .Case(MessageEntity::Type::kBotCommand, "bot_command")
    .Case(MessageEntity::Type::kUrl, "url")
    .Case(MessageEntity::Type::kEmail, "email")
    .Case(MessageEntity::Type::kPhoneNumber, "phone_number")
    .Case(MessageEntity::Type::kBold, "bold")
    .Case(MessageEntity::Type::kItalic, "italic")
    .Case(MessageEntity::Type::kUnderline, "underline")
    .Case(MessageEntity::Type::kStrikethrough, "strikethrough")
    .Case(MessageEntity::Type::kSpoiler, "spoiler")
    .Case(MessageEntity::Type::kCode, "code")
    .Case(MessageEntity::Type::kPre, "pre")
    .Case(MessageEntity::Type::kTextLink, "text_link")
    .Case(MessageEntity::Type::kTextMention, "text_mention")
    .Case(MessageEntity::Type::kCustomEmoji, "custom_emoji");
});

}  // namespace

namespace impl {

template <class Value>
MessageEntity Parse(const Value& data, formats::parse::To<MessageEntity>) {
  return MessageEntity{
    data["type"].template As<MessageEntity::Type>(),
    data["offset"].template As<std::int64_t>(),
    data["length"].template As<std::int64_t>(),
    data["url"].template As<std::optional<std::string>>(),
    data["user"].template As<std::unique_ptr<User>>(),
    data["language"].template As<std::optional<std::string>>(),
    data["custom_emoji_id"].template As<std::optional<std::string>>()
  };
}

template <class Value>
Value Serialize(const MessageEntity& message_entity, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["type"] = message_entity.type;
  builder["offset"] = message_entity.offset;
  builder["length"] = message_entity.length;
  SetIfNotNull(builder, "url", message_entity.url);
  SetIfNotNull(builder, "user", message_entity.user);
  SetIfNotNull(builder, "language", message_entity.language);
  SetIfNotNull(builder, "custom_emoji_id", message_entity.custom_emoji_id);
  return builder.ExtractValue();
}

}  // namespace impl

std::string_view ToString(MessageEntity::Type message_entity_type) {
  return utils::impl::EnumToStringView(message_entity_type, kMessageEntityTypeMap);
}

MessageEntity::Type Parse(const formats::json::Value& value,
                          formats::parse::To<MessageEntity::Type>) {
  return utils::ParseFromValueString(value, kMessageEntityTypeMap);
}

formats::json::Value Serialize(MessageEntity::Type message_entity_type,
                               formats::serialize::To<formats::json::Value>) {
  return formats::json::ValueBuilder(ToString(message_entity_type))
          .ExtractValue();
}

MessageEntity Parse(const formats::json::Value& json,
                    formats::parse::To<MessageEntity> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const MessageEntity& message_entity,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(message_entity, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
