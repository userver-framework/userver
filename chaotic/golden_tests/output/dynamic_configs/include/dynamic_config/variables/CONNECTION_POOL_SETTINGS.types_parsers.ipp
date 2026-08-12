#pragma once

#include "dynamic_config/variables/CONNECTION_POOL_SETTINGS.types.hpp"

#include <userver/chaotic/additional_properties.hpp>
#include <userver/chaotic/io/userver/utils/default_dict.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

namespace dynamic_config {namespace connection_pool_settings {

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
PoolSettings Parse(
    Value value,
    USERVER_NAMESPACE::formats::parse::To<PoolSettings>)
{
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    PoolSettings res{
        .min_pool_size = value["min_pool_size"].template As<
            USERVER_NAMESPACE::chaotic::WithType<USERVER_NAMESPACE::chaotic::Primitive<int, USERVER_NAMESPACE::chaotic::Minimum<::dynamic_config::connection_pool_settings::PoolSettings::kMin_Pool_SizeMinimum>>, std::size_t>
        >(USERVER_NAMESPACE::chaotic::ConvertTo<std::size_t>(1LL)),
        .max_pool_size = value["max_pool_size"].template As<
            USERVER_NAMESPACE::chaotic::WithType<USERVER_NAMESPACE::chaotic::Primitive<int, USERVER_NAMESPACE::chaotic::Minimum<::dynamic_config::connection_pool_settings::PoolSettings::kMax_Pool_SizeMinimum>>, std::size_t>
        >(USERVER_NAMESPACE::chaotic::ConvertTo<std::size_t>(10LL)),
    };

    return res;
}

constexpr USERVER_NAMESPACE::utils::TrivialSet k__dynamic_config__connection_pool_settings__VariableTypeRaw_PropertiesNames = [](auto selector) {
    return selector().template Type<std::string_view>()
        .Case("__default__")
    ;
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
VariableTypeRaw Parse(
    Value value,
    USERVER_NAMESPACE::formats::parse::To<VariableTypeRaw>)
{
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    auto extra = USERVER_NAMESPACE::chaotic::ExtractAdditionalProperties<
        USERVER_NAMESPACE::chaotic::Primitive<::dynamic_config::connection_pool_settings::PoolSettings>,
        std::unordered_map
    >(value, k__dynamic_config__connection_pool_settings__VariableTypeRaw_PropertiesNames);

    VariableTypeRaw res{
        .__default__ = value["__default__"].template As<
            std::optional<USERVER_NAMESPACE::chaotic::Primitive<::dynamic_config::connection_pool_settings::PoolSettings>>
        >(),
        .extra=std::move(extra),
    };

    return res;
}

}  // namespace connection_pool_settings
}  // namespace dynamic_config

