#pragma once

#include "oneof_fwd.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <userver/chaotic/object.hpp>
#include <userver/formats/parse/variant.hpp>
#include <variant>

#include <userver/chaotic/type_bundle_hpp.hpp>

namespace ns {

struct OneOf {
  using Foo = std::variant<int, std::string>;

  std::optional<ns::OneOf::Foo> foo{};
};

bool operator==(const ns::OneOf& lhs, const ns::OneOf& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh, const ns::OneOf& value);

OneOf Parse(USERVER_NAMESPACE::formats::json::Value json,
            USERVER_NAMESPACE::formats::parse::To<ns::OneOf>);

OneOf Parse(USERVER_NAMESPACE::formats::yaml::Value json,
            USERVER_NAMESPACE::formats::parse::To<ns::OneOf>);

OneOf Parse(USERVER_NAMESPACE::yaml_config::Value json,
            USERVER_NAMESPACE::formats::parse::To<ns::OneOf>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const ns::OneOf& value, USERVER_NAMESPACE::formats::serialize::To<
                                USERVER_NAMESPACE::formats::json::Value>);

}  // namespace ns
