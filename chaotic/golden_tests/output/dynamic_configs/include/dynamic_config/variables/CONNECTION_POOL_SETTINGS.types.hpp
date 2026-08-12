#pragma once

#include "dynamic_config/variables/CONNECTION_POOL_SETTINGS.types_fwd.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <userver/chaotic/io/std/size_t.hpp>
#include <userver/chaotic/io/std/unordered_map.hpp>
#include <userver/chaotic/object.hpp>
#include <userver/utils/default_dict.hpp>

#include <userver/chaotic/type_bundle_hpp.hpp>

namespace dynamic_config {namespace connection_pool_settings {

struct PoolSettings {
    static constexpr auto kMin_Pool_SizeMinimum = 0;
    static constexpr auto kMax_Pool_SizeMinimum = 1;
    static constexpr auto kFieldDefaultmin_pool_size = std::int64_t{1};
    static constexpr auto kFieldDefaultmax_pool_size = std::int64_t{10};

    std::size_t min_pool_size{USERVER_NAMESPACE::chaotic::ConvertTo<std::size_t>(1LL)};
    std::size_t max_pool_size{USERVER_NAMESPACE::chaotic::ConvertTo<std::size_t>(10LL)};
};

bool operator==(const PoolSettings & lhs,const PoolSettings & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const PoolSettings& value);

PoolSettings Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<PoolSettings>);

PoolSettings Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<PoolSettings>);

PoolSettings Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<PoolSettings>);

PoolSettings FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<PoolSettings>);

std::string ToJsonString(const PoolSettings& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const PoolSettings& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

struct VariableTypeRaw {
    std::optional<::dynamic_config::connection_pool_settings::PoolSettings> __default__{};
    std::unordered_map<std::string, ::dynamic_config::connection_pool_settings::PoolSettings> extra{};
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

}  // namespace connection_pool_settings
}  // namespace dynamic_config

