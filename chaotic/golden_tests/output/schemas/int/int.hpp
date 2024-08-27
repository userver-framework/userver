#pragma once

#include "int_fwd.hpp"

#include <cstdint>
#include <optional>
#include <userver/chaotic/object.hpp>

#include <userver/chaotic/type_bundle_hpp.hpp>

namespace ns {

struct Int {
  std::optional<int> foo{};
};

bool operator==(const ns::Int& lhs, const ns::Int& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh, const ns::Int& value);

Int Parse(USERVER_NAMESPACE::formats::json::Value json,
          USERVER_NAMESPACE::formats::parse::To<ns::Int>);

Int Parse(USERVER_NAMESPACE::formats::yaml::Value json,
          USERVER_NAMESPACE::formats::parse::To<ns::Int>);

Int Parse(USERVER_NAMESPACE::yaml_config::Value json,
          USERVER_NAMESPACE::formats::parse::To<ns::Int>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const ns::Int& value, USERVER_NAMESPACE::formats::serialize::To<
                              USERVER_NAMESPACE::formats::json::Value>);

}  // namespace ns
