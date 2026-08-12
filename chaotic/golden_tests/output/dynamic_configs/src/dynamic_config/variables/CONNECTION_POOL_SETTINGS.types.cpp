#include "dynamic_config/variables/CONNECTION_POOL_SETTINGS.types.hpp"

#include <userver/chaotic/type_bundle_cpp.hpp>

#include "dynamic_config/variables/CONNECTION_POOL_SETTINGS.types_parsers.ipp"

namespace dynamic_config {namespace connection_pool_settings {

PoolSettings FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<PoolSettings>)
{
    return USERVER_NAMESPACE::formats::json::FromString(json).As<PoolSettings>();
}

std::string ToJsonString(const PoolSettings& value) {
    return ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

bool operator==(const PoolSettings & lhs,const PoolSettings & rhs) {
    return true
        && lhs.min_pool_size == rhs.min_pool_size
        && lhs.max_pool_size == rhs.max_pool_size
    ;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh,
    const PoolSettings& value)
{
    return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

PoolSettings Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<PoolSettings> to)
{
    return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

PoolSettings Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<PoolSettings> to)
{
    return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

PoolSettings Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<PoolSettings> to)
{
    return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const PoolSettings& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>)
{
    USERVER_NAMESPACE::formats::json::ValueBuilder vb
            = USERVER_NAMESPACE::formats::common::Type::kObject;
    vb["min_pool_size"] =
        USERVER_NAMESPACE::chaotic::WithType<USERVER_NAMESPACE::chaotic::Primitive<int, USERVER_NAMESPACE::chaotic::Minimum<::dynamic_config::connection_pool_settings::PoolSettings::kMin_Pool_SizeMinimum>>, std::size_t>{
            value.min_pool_size
        };
    vb["max_pool_size"] =
        USERVER_NAMESPACE::chaotic::WithType<USERVER_NAMESPACE::chaotic::Primitive<int, USERVER_NAMESPACE::chaotic::Minimum<::dynamic_config::connection_pool_settings::PoolSettings::kMax_Pool_SizeMinimum>>, std::size_t>{
            value.max_pool_size
        };

    return vb.ExtractValue();
}

VariableTypeRaw FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<VariableTypeRaw>)
{
    return USERVER_NAMESPACE::formats::json::FromString(json).As<VariableTypeRaw>();
}

std::string ToJsonString(const VariableTypeRaw& value) {
    return ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

bool operator==(const VariableTypeRaw & lhs,const VariableTypeRaw & rhs) {
    return true
        && lhs.__default__ == rhs.__default__
        && lhs.extra == rhs.extra
    ;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh,
    const VariableTypeRaw& value)
{
    return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

VariableTypeRaw Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<VariableTypeRaw> to)
{
    return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

VariableTypeRaw Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<VariableTypeRaw> to)
{
    return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

VariableTypeRaw Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<VariableTypeRaw> to)
{
    return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const VariableTypeRaw& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>)
{
    USERVER_NAMESPACE::formats::json::ValueBuilder vb
            ;

            for (const auto&[field_key, field_value]: value.extra) {
                vb.EmplaceNocheck(field_key,
                    USERVER_NAMESPACE::chaotic::Primitive<::dynamic_config::connection_pool_settings::PoolSettings>{
                        field_value
                    }
                );
            }
    if (value.__default__) {
        vb["__default__"] =
            USERVER_NAMESPACE::chaotic::Primitive<::dynamic_config::connection_pool_settings::PoolSettings>{
                *value.__default__
            };
    }

    return vb.ExtractValue();
}

}  // namespace connection_pool_settings
}  // namespace dynamic_config

