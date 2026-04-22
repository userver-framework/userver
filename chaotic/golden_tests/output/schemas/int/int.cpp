#include <userver/chaotic/type_bundle_cpp.hpp>

#include "int.hpp"
#include "int_parsers.ipp"
#include "int_sax_parsers.hpp"

namespace ns {

Int FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<Int>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      Int, USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<USERVER_NAMESPACE::chaotic::sax::Parser<Int> > >(
      json);
}

bool operator==(const Int& lhs, const Int& rhs) { return lhs.foo == rhs.foo && true; }

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const Int& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

Int Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<Int> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

Int Parse(USERVER_NAMESPACE::formats::yaml::Value json, USERVER_NAMESPACE::formats::parse::To<Int> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

Int Parse(USERVER_NAMESPACE::yaml_config::Value json, USERVER_NAMESPACE::formats::parse::To<Int> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const Int& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  if (value.foo) {
    vb["foo"] = USERVER_NAMESPACE::chaotic::Primitive<int>{*value.foo};
  }

  return vb.ExtractValue();
}

}  // namespace ns

