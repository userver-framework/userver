#include "oneof.hpp"

#include <userver/chaotic/type_bundle_cpp.hpp>

#include "oneof_parsers.ipp"

namespace ns {

bool operator==(const ns::OneOf& lhs, const ns::OneOf& rhs) {
  return lhs.foo == rhs.foo && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh, const ns::OneOf& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value)
                            .ExtractValue());
}

OneOf Parse(USERVER_NAMESPACE::formats::json::Value json,
            USERVER_NAMESPACE::formats::parse::To<ns::OneOf> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

OneOf Parse(USERVER_NAMESPACE::formats::yaml::Value json,
            USERVER_NAMESPACE::formats::parse::To<ns::OneOf> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

OneOf Parse(USERVER_NAMESPACE::yaml_config::Value json,
            USERVER_NAMESPACE::formats::parse::To<ns::OneOf> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const ns::OneOf& value,
    USERVER_NAMESPACE::formats::serialize::To<
        USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb =
      USERVER_NAMESPACE::formats::common::Type::kObject;

  if (value.foo) {
    vb["foo"] = USERVER_NAMESPACE::chaotic::Variant<
        USERVER_NAMESPACE::chaotic::Primitive<int>,
        USERVER_NAMESPACE::chaotic::Primitive<std::string>>{*value.foo};
  }

  return vb.ExtractValue();
}

}  // namespace ns
