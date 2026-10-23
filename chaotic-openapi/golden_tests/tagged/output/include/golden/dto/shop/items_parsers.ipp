#pragma once

#include "golden/dto/shop/items.hpp"

#include <userver/chaotic/additional_properties.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

namespace golden {namespace dto {

constexpr USERVER_NAMESPACE::utils::TrivialSet k__golden__dto__Item_PropertiesNames = [](auto selector) {
    return selector().template Type<std::string_view>()
        .Case("id")
        .Case("title")
        .Case("last_error")
    ;
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
Item Parse(
    Value value,
    USERVER_NAMESPACE::formats::parse::To<Item>)
{
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    Item res{
        .id = value["id"].template As<
            USERVER_NAMESPACE::chaotic::Primitive<int>
        >(),
        .title = value["title"].template As<
            std::optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>
        >(),
        .last_error = value["last_error"].template As<
            std::optional<USERVER_NAMESPACE::chaotic::Primitive<::golden::errors::Error>>
        >(),
    };

    USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(
        value, k__golden__dto__Item_PropertiesNames
    );

    return res;
}

}  // namespace dto
}  // namespace golden

