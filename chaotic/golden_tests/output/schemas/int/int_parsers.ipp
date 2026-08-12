#pragma once

#include "int.hpp"

#include <userver/chaotic/additional_properties.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

namespace ns {

constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__Int_PropertiesNames = [](auto selector) {
    return selector().template Type<std::string_view>()
        .Case("foo")
    ;
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
Int Parse(
    Value value,
    USERVER_NAMESPACE::formats::parse::To<Int>)
{
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    Int res{
        .foo = value["foo"].template As<
            std::optional<USERVER_NAMESPACE::chaotic::Primitive<int>>
        >(),
    };

    USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(
        value, k__ns__Int_PropertiesNames
    );

    return res;
}

}  // namespace ns

