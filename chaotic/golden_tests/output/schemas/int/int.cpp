#include "int.hpp"

#include <userver/chaotic/type_bundle_cpp.hpp>

#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

#include "int_parsers.ipp"

namespace ns {

bool operator==(const ns::Int& lhs, const ns::Int& rhs) {
  return lhs.foo == rhs.foo && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh, const ns::Int& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value)
                            .ExtractValue());
}

Int Parse(USERVER_NAMESPACE::formats::json::Value json,
          USERVER_NAMESPACE::formats::parse::To<ns::Int> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

Int Parse(USERVER_NAMESPACE::formats::yaml::Value json,
          USERVER_NAMESPACE::formats::parse::To<ns::Int> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

Int Parse(USERVER_NAMESPACE::yaml_config::Value json,
          USERVER_NAMESPACE::formats::parse::To<ns::Int> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const ns::Int& value,
    USERVER_NAMESPACE::formats::serialize::To<
        USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb =
      USERVER_NAMESPACE::formats::common::Type::kObject;

  if (value.foo) {
    vb["foo"] = USERVER_NAMESPACE::chaotic::Primitive<int>{*value.foo};
  }

  return vb.ExtractValue();
}

}  // namespace ns
