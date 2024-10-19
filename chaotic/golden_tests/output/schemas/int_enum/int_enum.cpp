#include "int_enum.hpp"

#include <userver/chaotic/type_bundle_cpp.hpp>

#include "int_enum_parsers.ipp"

namespace ns {

bool operator==(const ns::Enum& lhs, const ns::Enum& rhs) {
  return lhs.foo == rhs.foo && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh, const ns::Enum::Foo& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value)
                            .ExtractValue());
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh, const ns::Enum& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value)
                            .ExtractValue());
}

Enum::Foo Parse(USERVER_NAMESPACE::formats::json::Value json,
                USERVER_NAMESPACE::formats::parse::To<ns::Enum::Foo> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

Enum Parse(USERVER_NAMESPACE::formats::json::Value json,
           USERVER_NAMESPACE::formats::parse::To<ns::Enum> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

Enum::Foo Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                USERVER_NAMESPACE::formats::parse::To<ns::Enum::Foo> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

Enum Parse(USERVER_NAMESPACE::formats::yaml::Value json,
           USERVER_NAMESPACE::formats::parse::To<ns::Enum> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

Enum::Foo Parse(USERVER_NAMESPACE::yaml_config::Value json,
                USERVER_NAMESPACE::formats::parse::To<ns::Enum::Foo> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

Enum Parse(USERVER_NAMESPACE::yaml_config::Value json,
           USERVER_NAMESPACE::formats::parse::To<ns::Enum> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    const ns::Enum::Foo& value, USERVER_NAMESPACE::formats::serialize::To<
                                    USERVER_NAMESPACE::formats::json::Value>
                                    to) {
  const auto result = kns__Enum__Foo_Mapping.TryFindByFirst(value);
  if (result.has_value()) {
    return Serialize(*result, to);
  }
  throw std::runtime_error("Bad enum value");
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const ns::Enum& value,
    USERVER_NAMESPACE::formats::serialize::To<
        USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb =
      USERVER_NAMESPACE::formats::common::Type::kObject;

  if (value.foo) {
    vb["foo"] =
        USERVER_NAMESPACE::chaotic::Primitive<ns::Enum::Foo>{*value.foo};
  }

  return vb.ExtractValue();
}

}  // namespace ns
