#pragma once

#include "enum.hpp"

#include <userver/chaotic/additional_properties.hpp>
#include <userver/chaotic/exception.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/common/meta.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

namespace ns {

constexpr USERVER_NAMESPACE::utils::TrivialBiMap k__ns__Enum__Foo_Mapping = [](auto selector) {
    return selector().template Type<Enum::Foo, std::string_view>()
        .Case(Enum::Foo::kOne, "one")
        .Case(Enum::Foo::kTwo, "two")
        .Case(Enum::Foo::kThree, "three")
    ;
};

constexpr USERVER_NAMESPACE::utils::TrivialBiMap k__ns__Enum__With_Fallback_Mapping = [](auto selector) {
    return selector().template Type<Enum::With_Fallback, std::string_view>()
        .Case(Enum::With_Fallback::kUnknown, "unknown")
        .Case(Enum::With_Fallback::kOne, "one")
        .Case(Enum::With_Fallback::kTwo, "two")
    ;
};

constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__Enum_PropertiesNames = [](auto selector) {
    return selector().template Type<std::string_view>()
        .Case("foo")
        .Case("with_fallback")
    ;
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
Enum::Foo Parse(
    Value val,
    USERVER_NAMESPACE::formats::parse::To<Enum::Foo>)
{
    const auto value = val.template As<std::string>();
    const auto result = k__ns__Enum__Foo_Mapping.TryFindBySecond(value);
    if (result.has_value()) {
        return *result;
    }
    USERVER_NAMESPACE::chaotic::ThrowForValue(fmt::format("Invalid enum value ({}) for type ::ns::Enum::Foo", value), val);
}

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
Enum::With_Fallback Parse(
    Value val,
    USERVER_NAMESPACE::formats::parse::To<Enum::With_Fallback>)
{
    const auto value = val.template As<std::string>();
    const auto result = k__ns__Enum__With_Fallback_Mapping.TryFindBySecond(value);
    if (result.has_value()) {
        return *result;
    }
    return Enum::With_Fallback::kUnknown;
}

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
Enum Parse(
    Value value,
    USERVER_NAMESPACE::formats::parse::To<Enum>)
{
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    Enum res{
        .foo = value["foo"].template As<
            std::optional<USERVER_NAMESPACE::chaotic::Primitive<::ns::Enum::Foo>>
        >(),
        .with_fallback = value["with_fallback"].template As<
            std::optional<USERVER_NAMESPACE::chaotic::Primitive<::ns::Enum::With_Fallback>>
        >(),
    };

    USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(
        value, k__ns__Enum_PropertiesNames
    );

    return res;
}

}  // namespace ns

