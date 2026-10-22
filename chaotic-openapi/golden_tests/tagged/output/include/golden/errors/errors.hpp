#pragma once

#include "golden/errors/errors_fwd.hpp"

#include <optional>
#include <string>
#include <userver/chaotic/object.hpp>

#include <userver/chaotic/type_bundle_hpp.hpp>

namespace golden {namespace errors {

struct Error {
    std::string code{};
    std::optional<std::string> message{};
};

bool operator==(const Error & lhs,const Error & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const Error& value);

Error Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<Error>);

Error FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<Error>);

std::string ToJsonString(const Error& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const Error& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const Error& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

}  // namespace errors
}  // namespace golden

