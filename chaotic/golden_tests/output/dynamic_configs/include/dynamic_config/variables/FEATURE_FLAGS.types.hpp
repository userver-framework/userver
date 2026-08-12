#pragma once

#include "dynamic_config/variables/FEATURE_FLAGS.types_fwd.hpp"

#include <cstdint>
#include <optional>
#include <userver/chaotic/io/std/chrono/milliseconds.hpp>
#include <userver/chaotic/object.hpp>

#include <userver/chaotic/type_bundle_hpp.hpp>

namespace dynamic_config {namespace feature_flags {

struct VariableTypeRaw {
    static constexpr auto kTimeout_MsMinimum = 0;
    bool enable_new_logic{};
    std::optional<std::chrono::milliseconds> timeout_ms{};
};

bool operator==(const VariableTypeRaw & lhs,const VariableTypeRaw & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const VariableTypeRaw& value);

VariableTypeRaw Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<VariableTypeRaw>);

VariableTypeRaw Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<VariableTypeRaw>);

VariableTypeRaw Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<VariableTypeRaw>);

VariableTypeRaw FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<VariableTypeRaw>);

std::string ToJsonString(const VariableTypeRaw& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const VariableTypeRaw& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

}  // namespace feature_flags
}  // namespace dynamic_config

