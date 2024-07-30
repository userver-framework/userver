#include "allof.hpp"

#include <userver/chaotic/type_bundle_cpp.hpp>

#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/common/merge.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

#include "allof_parsers.ipp"

namespace ns {

bool operator==(const ns::AllOf::Foo__P0& lhs, const ns::AllOf::Foo__P0& rhs) {
  return lhs.foo == rhs.foo && lhs.extra == rhs.extra &&

         true;
}

bool operator==(const ns::AllOf::Foo__P1& lhs, const ns::AllOf::Foo__P1& rhs) {
  return lhs.bar == rhs.bar && lhs.extra == rhs.extra &&

         true;
}

bool operator==(const ns::AllOf::Foo& lhs, const ns::AllOf::Foo& rhs) {
  return static_cast<const ns::AllOf::Foo__P0&>(lhs) ==
             static_cast<const ns::AllOf::Foo__P0&>(rhs) &&
         static_cast<const ns::AllOf::Foo__P1&>(lhs) ==
             static_cast<const ns::AllOf::Foo__P1&>(rhs);
}

bool operator==(const ns::AllOf& lhs, const ns::AllOf& rhs) {
  return lhs.foo == rhs.foo && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh,
    const ns::AllOf::Foo__P0& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value)
                            .ExtractValue());
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh,
    const ns::AllOf::Foo__P1& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value)
                            .ExtractValue());
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh, const ns::AllOf::Foo& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value)
                            .ExtractValue());
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh, const ns::AllOf& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value)
                            .ExtractValue());
}

AllOf::Foo__P0 Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<ns::AllOf::Foo__P0> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

AllOf::Foo__P1 Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<ns::AllOf::Foo__P1> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

AllOf::Foo Parse(USERVER_NAMESPACE::formats::json::Value json,
                 USERVER_NAMESPACE::formats::parse::To<ns::AllOf::Foo> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

AllOf Parse(USERVER_NAMESPACE::formats::json::Value json,
            USERVER_NAMESPACE::formats::parse::To<ns::AllOf> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

/* Parse(USERVER_NAMESPACE::formats::yaml::Value, To<ns::AllOf>) was not
 * generated: ns::AllOf@Foo__P0 has JSON-specific field "extra" */

/* Parse(USERVER_NAMESPACE::yaml_config::Value, To<ns::AllOf>) was not
 * generated: ns::AllOf@Foo__P0 has JSON-specific field "extra" */

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const ns::AllOf::Foo__P0& value,
    USERVER_NAMESPACE::formats::serialize::To<
        USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = value.extra;

  if (value.foo) {
    vb["foo"] = USERVER_NAMESPACE::chaotic::Primitive<std::string>{*value.foo};
  }

  return vb.ExtractValue();
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const ns::AllOf::Foo__P1& value,
    USERVER_NAMESPACE::formats::serialize::To<
        USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = value.extra;

  if (value.bar) {
    vb["bar"] = USERVER_NAMESPACE::chaotic::Primitive<int>{*value.bar};
  }

  return vb.ExtractValue();
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    const ns::AllOf::Foo& value, USERVER_NAMESPACE::formats::serialize::To<
                                     USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb =
      USERVER_NAMESPACE::formats::common::Type::kObject;
  USERVER_NAMESPACE::formats::common::Merge(
      vb,
      USERVER_NAMESPACE::formats::json::ValueBuilder{
          static_cast<ns::AllOf::Foo__P0>(value)}
          .ExtractValue());
  USERVER_NAMESPACE::formats::common::Merge(
      vb,
      USERVER_NAMESPACE::formats::json::ValueBuilder{
          static_cast<ns::AllOf::Foo__P1>(value)}
          .ExtractValue());
  return vb.ExtractValue();
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const ns::AllOf& value,
    USERVER_NAMESPACE::formats::serialize::To<
        USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb =
      USERVER_NAMESPACE::formats::common::Type::kObject;

  if (value.foo) {
    vb["foo"] =
        USERVER_NAMESPACE::chaotic::Primitive<ns::AllOf::Foo>{*value.foo};
  }

  return vb.ExtractValue();
}

}  // namespace ns
