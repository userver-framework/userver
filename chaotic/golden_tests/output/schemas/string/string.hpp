#pragma once

#include <optional>
#include <string>
#include <userver/chaotic/type_bundle_hpp.hpp>

#include "string_fwd.hpp"

namespace ns {

struct String {
    std::optional<std::string> foo{};
};

bool operator==(const String& lhs, const String& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const String& value);

String Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<String>);

String Parse(USERVER_NAMESPACE::formats::yaml::Value json, USERVER_NAMESPACE::formats::parse::To<String>);

String Parse(USERVER_NAMESPACE::yaml_config::Value json, USERVER_NAMESPACE::formats::parse::To<String>);

USERVER_NAMESPACE::formats::json::Value
Serialize(const String& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

}  // namespace ns
