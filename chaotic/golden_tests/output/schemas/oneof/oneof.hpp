#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <userver/chaotic/object.hpp>
#include <userver/chaotic/type_bundle_hpp.hpp>
#include <userver/formats/json/serialize_variant.hpp>
#include <userver/formats/parse/variant.hpp>
#include <variant>

#include "oneof_fwd.hpp"

namespace ns {

struct OneOf {
  using Foo = std::variant<int, std::string>;

  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamefoo = "foo";
  std::optional<::ns::OneOf::Foo> foo{};
};

bool operator==(const OneOf& lhs, const OneOf& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const OneOf& value);

OneOf Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<OneOf>);

OneOf Parse(USERVER_NAMESPACE::formats::yaml::Value json, USERVER_NAMESPACE::formats::parse::To<OneOf>);

OneOf Parse(USERVER_NAMESPACE::yaml_config::Value json, USERVER_NAMESPACE::formats::parse::To<OneOf>);

OneOf FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<OneOf>);

std::string ToJsonString(const OneOf& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const OneOf& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(const ::ns::OneOf& value, USERVER_NAMESPACE::formats::json::StringBuilder& sw,
                   bool hide_brackets = false, std::string_view hide_field_name = {});

}  // namespace ns

