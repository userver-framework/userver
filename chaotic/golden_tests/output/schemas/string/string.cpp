#include "string.hpp"

#include <userver/chaotic/type_bundle_cpp.hpp>

#include "string_parsers.ipp"

namespace ns {

bool operator==(const ns::String& lhs, const ns::String& rhs) {
  return lhs.foo == rhs.foo && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh, const ns::String& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value)
                            .ExtractValue());
}

String Parse(USERVER_NAMESPACE::formats::json::Value json,
             USERVER_NAMESPACE::formats::parse::To<ns::String> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

String Parse(USERVER_NAMESPACE::formats::yaml::Value json,
             USERVER_NAMESPACE::formats::parse::To<ns::String> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

String Parse(USERVER_NAMESPACE::yaml_config::Value json,
             USERVER_NAMESPACE::formats::parse::To<ns::String> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const ns::String& value,
    USERVER_NAMESPACE::formats::serialize::To<
        USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb =
      USERVER_NAMESPACE::formats::common::Type::kObject;

  if (value.foo) {
    vb["foo"] = USERVER_NAMESPACE::chaotic::Primitive<std::string>{*value.foo};
  }

  return vb.ExtractValue();
}

}  // namespace ns
