#include "dynamic_config/variables/FEATURE_FLAGS.types.hpp"

#include <userver/chaotic/type_bundle_cpp.hpp>

#include "dynamic_config/variables/FEATURE_FLAGS.types_parsers.ipp"

namespace dynamic_config {namespace feature_flags {

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
        && lhs.enable_new_logic == rhs.enable_new_logic
        && lhs.timeout_ms == rhs.timeout_ms
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
            = USERVER_NAMESPACE::formats::common::Type::kObject;
    vb["enable_new_logic"] =
        USERVER_NAMESPACE::chaotic::Primitive<bool>{
            value.enable_new_logic
        };
    if (value.timeout_ms) {
        vb["timeout_ms"] =
            USERVER_NAMESPACE::chaotic::WithType<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t, USERVER_NAMESPACE::chaotic::Minimum<::dynamic_config::feature_flags::VariableTypeRaw::kTimeout_MsMinimum>>, std::chrono::milliseconds>{
                *value.timeout_ms
            };
    }

    return vb.ExtractValue();
}

}  // namespace feature_flags
}  // namespace dynamic_config

