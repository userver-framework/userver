#pragma once

#include "golden/errors/errors.hpp"

#include <userver/chaotic/additional_properties.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

namespace golden {namespace errors {

constexpr USERVER_NAMESPACE::utils::TrivialSet k__golden__errors__Error_PropertiesNames = [](auto selector) {
    return selector().template Type<std::string_view>()
        .Case("code")
        .Case("message")
    ;
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
Error Parse(
    Value value,
    USERVER_NAMESPACE::formats::parse::To<Error>)
{
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    Error res{
        .code = value["code"].template As<
            USERVER_NAMESPACE::chaotic::Primitive<std::string>
        >(),
        .message = value["message"].template As<
            std::optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>
        >(),
    };

    USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(
        value, k__golden__errors__Error_PropertiesNames
    );

    return res;
}

}  // namespace errors
}  // namespace golden

