#pragma once

#include <cstdint>
#include <optional>
#include <userver/chaotic/object.hpp>
#include <userver/chaotic/type_bundle_hpp.hpp>

#include "int_fwd.hpp"

namespace ns {

struct Int {
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamefoo = "foo";
  std::optional<int> foo{};
};

bool operator==(const Int& lhs, const Int& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const Int& value);

Int Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<Int>);

Int Parse(USERVER_NAMESPACE::formats::yaml::Value json, USERVER_NAMESPACE::formats::parse::To<Int>);

Int Parse(USERVER_NAMESPACE::yaml_config::Value json, USERVER_NAMESPACE::formats::parse::To<Int>);

Int FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<Int>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const Int& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

}  // namespace ns

