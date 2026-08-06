#pragma once

#include "any_fwd.hpp"

#include <optional>
#include <userver/chaotic/object.hpp>
#include <userver/formats/json/value.hpp>

#include <userver/chaotic/type_bundle_hpp.hpp>

namespace ns {

using AnyValue = USERVER_NAMESPACE::formats::json::Value;

struct WithAnyField {
    std::optional<USERVER_NAMESPACE::formats::json::Value> payload{};
};

bool operator==(const WithAnyField & lhs,const WithAnyField & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const WithAnyField& value);

WithAnyField Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<WithAnyField>);

WithAnyField Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<WithAnyField>);

WithAnyField Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<WithAnyField>);

WithAnyField FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<WithAnyField>);

std::string ToJsonString(const WithAnyField& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const WithAnyField& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const WithAnyField& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

}  // namespace ns

