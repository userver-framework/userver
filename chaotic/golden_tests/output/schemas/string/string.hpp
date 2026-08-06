#pragma once

#include "string_fwd.hpp"

#include <optional>
#include <string>
#include <userver/chaotic/object.hpp>

#include <userver/chaotic/type_bundle_hpp.hpp>

namespace ns {

struct String {
    std::optional<std::string> foo{};
};

bool operator==(const String & lhs,const String & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const String& value);

String Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<String>);

String Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<String>);

String Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<String>);

String FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<String>);

std::string ToJsonString(const String& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const String& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const String& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

}  // namespace ns

