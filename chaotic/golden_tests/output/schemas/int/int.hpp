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

bool operator==(const Int & lhs,const Int & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const Int& value);

Int Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<Int>);

Int Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<Int>);

Int Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<Int>);

Int FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<Int>);

std::string ToJsonString(const Int& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const Int& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const Int& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

}  // namespace ns

