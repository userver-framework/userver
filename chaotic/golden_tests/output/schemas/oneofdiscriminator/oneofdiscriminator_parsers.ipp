#pragma once

#include "oneofdiscriminator.hpp"

#include <userver/chaotic/additional_properties.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/json/serialize_variant.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

namespace ns {

constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__A_PropertiesNames = [](auto selector) {
    return selector().template Type<std::string_view>()
        .Case("type")
        .Case("a_prop")
    ;
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
A Parse(
    Value value,
    USERVER_NAMESPACE::formats::parse::To<A>)
{
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    auto extra = USERVER_NAMESPACE::chaotic::ExtractAdditionalPropertiesTrue(
        value, k__ns__A_PropertiesNames
    );

    A res{
        .type = value["type"].template As<
            std::optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>
        >(),
        .a_prop = value["a_prop"].template As<
            std::optional<USERVER_NAMESPACE::chaotic::Primitive<int>>
        >(),
        .extra=std::move(extra),
    };

    return res;
}

constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__B_PropertiesNames = [](auto selector) {
    return selector().template Type<std::string_view>()
        .Case("type")
        .Case("b_prop")
    ;
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
B Parse(
    Value value,
    USERVER_NAMESPACE::formats::parse::To<B>)
{
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    auto extra = USERVER_NAMESPACE::chaotic::ExtractAdditionalPropertiesTrue(
        value, k__ns__B_PropertiesNames
    );

    B res{
        .type = value["type"].template As<
            std::optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>
        >(),
        .b_prop = value["b_prop"].template As<
            std::optional<USERVER_NAMESPACE::chaotic::Primitive<int>>
        >(),
        .extra=std::move(extra),
    };

    return res;
}

constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__C_PropertiesNames = [](auto selector) {
    return selector().template Type<std::string_view>()
        .Case("version")
    ;
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
C Parse(
    Value value,
    USERVER_NAMESPACE::formats::parse::To<C>)
{
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    C res{
        .version = value["version"].template As<
            std::optional<USERVER_NAMESPACE::chaotic::Primitive<int>>
        >(),
    };

    USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(
        value, k__ns__C_PropertiesNames
    );

    return res;
}

constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__D_PropertiesNames = [](auto selector) {
    return selector().template Type<std::string_view>()
        .Case("version")
    ;
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
D Parse(
    Value value,
    USERVER_NAMESPACE::formats::parse::To<D>)
{
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    D res{
        .version = value["version"].template As<
            std::optional<USERVER_NAMESPACE::chaotic::Primitive<int>>
        >(),
    };

    USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(
        value, k__ns__D_PropertiesNames
    );

    return res;
}

constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__IntegerOneOfDiscriminator_PropertiesNames = [](auto selector) {
    return selector().template Type<std::string_view>()
        .Case("foo")
    ;
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
IntegerOneOfDiscriminator Parse(
    Value value,
    USERVER_NAMESPACE::formats::parse::To<IntegerOneOfDiscriminator>)
{
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    IntegerOneOfDiscriminator res{
        .foo = value["foo"].template As<
            std::optional<USERVER_NAMESPACE::chaotic::OneOfWithDiscriminator<&::ns::IntegerOneOfDiscriminator::kFoo_Settings, USERVER_NAMESPACE::chaotic::Primitive<::ns::C>, USERVER_NAMESPACE::chaotic::Primitive<::ns::D>>>
        >(),
    };

    USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(
        value, k__ns__IntegerOneOfDiscriminator_PropertiesNames
    );

    return res;
}

constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__OneOfDiscriminator_PropertiesNames = [](auto selector) {
    return selector().template Type<std::string_view>()
        .Case("foo")
    ;
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
OneOfDiscriminator Parse(
    Value value,
    USERVER_NAMESPACE::formats::parse::To<OneOfDiscriminator>)
{
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    OneOfDiscriminator res{
        .foo = value["foo"].template As<
            std::optional<USERVER_NAMESPACE::chaotic::OneOfWithDiscriminator<&::ns::OneOfDiscriminator::kFoo_Settings, USERVER_NAMESPACE::chaotic::Primitive<::ns::A>, USERVER_NAMESPACE::chaotic::Primitive<::ns::B>>>
        >(),
    };

    USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(
        value, k__ns__OneOfDiscriminator_PropertiesNames
    );

    return res;
}

}  // namespace ns

