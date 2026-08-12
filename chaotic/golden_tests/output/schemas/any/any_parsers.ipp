#pragma once

#include "any.hpp"

#include <userver/chaotic/additional_properties.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

namespace ns {

constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__WithAnyField_PropertiesNames = [](auto selector) {
    return selector().template Type<std::string_view>()
        .Case("payload")
    ;
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
WithAnyField Parse(
    Value value,
    USERVER_NAMESPACE::formats::parse::To<WithAnyField>)
{
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    WithAnyField res{
        .payload = value["payload"].template As<
            std::optional<USERVER_NAMESPACE::formats::json::Value>
        >(),
    };

    USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(
        value, k__ns__WithAnyField_PropertiesNames
    );

    return res;
}

}  // namespace ns

