#include <userver/telegram/bot/types/sticker.hpp>

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

constexpr utils::TrivialBiMap kStickerTypeMap([](auto selector) {
  return selector()
    .Case(Sticker::Type::kRegular, "regular")
    .Case(Sticker::Type::kMask, "mask")
    .Case(Sticker::Type::kCustomEmoji, "custom_emoji");
});

}  // namespace 

namespace impl {

template <class Value>
Sticker Parse(const Value& data, formats::parse::To<Sticker>) {
  return Sticker{
    data["file_id"].template As<std::string>(),
    data["file_unique_id"].template As<std::string>(),
    data["type"].template As<Sticker::Type>(),
    data["width"].template As<std::int64_t>(),
    data["height"].template As<std::int64_t>(),
    data["is_animated"].template As<bool>(),
    data["is_video"].template As<bool>(),
    data["thumbnail"].template As<std::unique_ptr<PhotoSize>>(),
    data["emoji"].template As<std::optional<std::string>>(),
    data["set_name"].template As<std::optional<std::string>>(),
    data["premium_animation"].template As<std::unique_ptr<File>>(),
    data["mask_position"].template As<std::unique_ptr<MaskPosition>>(),
    data["custom_emoji_id"].template As<std::optional<std::string>>(),
    data["needs_repainting"].template As<std::optional<bool>>(),
    data["file_size"].template As<std::optional<std::int64_t>>()
  };
}

template <class Value>
Value Serialize(const Sticker& sticker, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["file_id"] = sticker.file_id;
  builder["file_unique_id"] = sticker.file_unique_id;
  builder["type"] = sticker.type;
  builder["width"] = sticker.width;
  builder["height"] = sticker.height;
  builder["is_animated"] = sticker.is_animated;
  builder["is_video"] = sticker.is_video;
  SetIfNotNull(builder, "thumbnail", sticker.thumbnail);
  SetIfNotNull(builder, "emoji", sticker.emoji);
  SetIfNotNull(builder, "set_name", sticker.set_name);
  SetIfNotNull(builder, "premium_animation", sticker.premium_animation);
  SetIfNotNull(builder, "mask_position", sticker.mask_position);
  SetIfNotNull(builder, "custom_emoji_id", sticker.custom_emoji_id);
  SetIfNotNull(builder, "needs_repainting", sticker.needs_repainting);
  SetIfNotNull(builder, "file_size", sticker.file_size);
  return builder.ExtractValue();
}

}  // namespace impl

std::string_view ToString(Sticker::Type sticker_type) {
  return utils::impl::EnumToStringView(sticker_type, kStickerTypeMap);
}

Sticker::Type Parse(const formats::json::Value& value,
                    formats::parse::To<Sticker::Type>) {
  return utils::ParseFromValueString(value, kStickerTypeMap);
}

formats::json::Value Serialize(Sticker::Type sticker_type,
                               formats::serialize::To<formats::json::Value>) {
  return formats::json::ValueBuilder(ToString(sticker_type)).ExtractValue();
}

Sticker Parse(const formats::json::Value& json,
              formats::parse::To<Sticker> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const Sticker& sticker,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(sticker, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
