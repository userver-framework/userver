#include <userver/telegram/bot/types/mask_position.hpp>

#include <userver/formats/json.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace {

constexpr utils::TrivialBiMap kMaskPositionPointMap([](auto selector) {
  return selector()
    .Case(MaskPosition::Point::kForehead, "forehead")
    .Case(MaskPosition::Point::kEyes, "eyes")
    .Case(MaskPosition::Point::kMouth, "mouth")
    .Case(MaskPosition::Point::kChin, "chin");
});

}  // namespace

namespace impl {

template <class Value>
MaskPosition Parse(const Value& data, formats::parse::To<MaskPosition>) {
  return MaskPosition{
    data["point"].template As<MaskPosition::Point>(),
    data["x_shift"].template As<double>(),
    data["y_shift"].template As<double>(),
    data["scale"].template As<double>()
  };
}

template <class Value>
Value Serialize(const MaskPosition& mask_position, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["point"] = mask_position.point;
  builder["x_shift"] = mask_position.x_shift;
  builder["y_shift"] = mask_position.y_shift;
  builder["scale"] = mask_position.scale;
  return builder.ExtractValue();
}

}  // namespace impl

std::string_view ToString(MaskPosition::Point point) {
  return utils::impl::EnumToStringView(point, kMaskPositionPointMap);
}

MaskPosition::Point Parse(const formats::json::Value& value,
                          formats::parse::To<MaskPosition::Point>) {
  return utils::ParseFromValueString(value, kMaskPositionPointMap);
}

formats::json::Value Serialize(MaskPosition::Point point,
                               formats::serialize::To<formats::json::Value>) {
  return formats::json::ValueBuilder(ToString(point)).ExtractValue();
}

MaskPosition Parse(const formats::json::Value& json,
                   formats::parse::To<MaskPosition> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const MaskPosition& mask_position,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(mask_position, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
