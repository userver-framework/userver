#pragma once

#include "allof.hpp"

#include <userver/chaotic/additional_properties.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/common/merge.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

namespace ns {

constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__AllOf__Foo__P0_PropertiesNames = [](auto selector) {
    return selector().template Type<std::string_view>()
        .Case("foo")
    ;
};

constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__AllOf__Foo__P1_PropertiesNames = [](auto selector) {
    return selector().template Type<std::string_view>()
        .Case("bar")
    ;
};

constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__AllOf_PropertiesNames = [](auto selector) {
    return selector().template Type<std::string_view>()
        .Case("foo")
    ;
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
AllOf::Foo__P0 Parse(
    Value value,
    USERVER_NAMESPACE::formats::parse::To<AllOf::Foo__P0>)
{
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    auto extra = USERVER_NAMESPACE::chaotic::ExtractAdditionalPropertiesTrue(
        value, k__ns__AllOf__Foo__P0_PropertiesNames
    );

    AllOf::Foo__P0 res{
        .foo = value["foo"].template As<
            std::optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>
        >(),
        .extra=std::move(extra),
    };

    return res;
}

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
AllOf::Foo__P1 Parse(
    Value value,
    USERVER_NAMESPACE::formats::parse::To<AllOf::Foo__P1>)
{
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    auto extra = USERVER_NAMESPACE::chaotic::ExtractAdditionalPropertiesTrue(
        value, k__ns__AllOf__Foo__P1_PropertiesNames
    );

    AllOf::Foo__P1 res{
        .bar = value["bar"].template As<
            std::optional<USERVER_NAMESPACE::chaotic::Primitive<int>>
        >(),
        .extra=std::move(extra),
    };

    return res;
}

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
AllOf::Foo Parse(
    Value value,
    USERVER_NAMESPACE::formats::parse::To<AllOf::Foo>)
{
    constexpr USERVER_NAMESPACE::utils::TrivialSet kPropertiesNames = [](auto selector) {
        return selector().template Type<std::string_view>()
            .Case("foo")
            .Case("bar")
        ;
    };

    auto extra = USERVER_NAMESPACE::chaotic::ExtractAdditionalPropertiesTrue(value, kPropertiesNames);

    return AllOf::Foo(
        AllOf::Foo__P0{
            .foo = value["foo"].template As<
                std::optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>
            >(),
            .extra=extra,
        },
        AllOf::Foo__P1{
            .bar = value["bar"].template As<
                std::optional<USERVER_NAMESPACE::chaotic::Primitive<int>>
            >(),
            .extra=extra,
        }
    );
}

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
AllOf Parse(
    Value value,
    USERVER_NAMESPACE::formats::parse::To<AllOf>)
{
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    AllOf res{
        .foo = value["foo"].template As<
            std::optional<USERVER_NAMESPACE::chaotic::Primitive<::ns::AllOf::Foo>>
        >(),
    };

    USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(
        value, k__ns__AllOf_PropertiesNames
    );

    return res;
}

}  // namespace ns

