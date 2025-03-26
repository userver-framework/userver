#pragma once

#include "oneofdiscriminator.hpp"

#include <userver/chaotic/object.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/json/serialize_variant.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

namespace ns {

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__A_PropertiesNames = [](auto selector) {
    return selector().template Type<std::string_view>().Case("type").Case("a_prop");
};

template <typename Value>
::ns::A Parse(Value value, USERVER_NAMESPACE::formats::parse::To<::ns::A>) {
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    ::ns::A res;

    res.type = value["type"].template As<std::optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>>();
    res.a_prop = value["a_prop"].template As<std::optional<USERVER_NAMESPACE::chaotic::Primitive<int>>>();

    res.extra = USERVER_NAMESPACE::chaotic::ExtractAdditionalPropertiesTrue(value, k__ns__A_PropertiesNames);

    return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__B_PropertiesNames = [](auto selector) {
    return selector().template Type<std::string_view>().Case("type").Case("b_prop");
};

template <typename Value>
::ns::B Parse(Value value, USERVER_NAMESPACE::formats::parse::To<::ns::B>) {
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    ::ns::B res;

    res.type = value["type"].template As<std::optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>>();
    res.b_prop = value["b_prop"].template As<std::optional<USERVER_NAMESPACE::chaotic::Primitive<int>>>();

    res.extra = USERVER_NAMESPACE::chaotic::ExtractAdditionalPropertiesTrue(value, k__ns__B_PropertiesNames);

    return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__C_PropertiesNames = [](auto selector) {
    return selector().template Type<std::string_view>().Case("version");
};

template <typename Value>
::ns::C Parse(Value value, USERVER_NAMESPACE::formats::parse::To<::ns::C>) {
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    ::ns::C res;

    res.version = value["version"].template As<std::optional<USERVER_NAMESPACE::chaotic::Primitive<int>>>();

    USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value, k__ns__C_PropertiesNames);

    return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__D_PropertiesNames = [](auto selector) {
    return selector().template Type<std::string_view>().Case("version");
};

template <typename Value>
::ns::D Parse(Value value, USERVER_NAMESPACE::formats::parse::To<::ns::D>) {
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    ::ns::D res;

    res.version = value["version"].template As<std::optional<USERVER_NAMESPACE::chaotic::Primitive<int>>>();

    USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value, k__ns__D_PropertiesNames);

    return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__IntegerOneOfDiscriminator_PropertiesNames =
    [](auto selector) { return selector().template Type<std::string_view>().Case("foo"); };

template <typename Value>
::ns::IntegerOneOfDiscriminator
Parse(Value value, USERVER_NAMESPACE::formats::parse::To<::ns::IntegerOneOfDiscriminator>) {
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    ::ns::IntegerOneOfDiscriminator res;

    res.foo = value["foo"]
                  .template As<std::optional<USERVER_NAMESPACE::chaotic::OneOfWithDiscriminator<
                      &::ns::IntegerOneOfDiscriminator::kFoo_Settings,
                      USERVER_NAMESPACE::chaotic::Primitive<::ns::C>,
                      USERVER_NAMESPACE::chaotic::Primitive<::ns::D>>>>();

    USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value, k__ns__IntegerOneOfDiscriminator_PropertiesNames);

    return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__OneOfDiscriminator_PropertiesNames = [](auto selector) {
    return selector().template Type<std::string_view>().Case("foo");
};

template <typename Value>
::ns::OneOfDiscriminator Parse(Value value, USERVER_NAMESPACE::formats::parse::To<::ns::OneOfDiscriminator>) {
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    ::ns::OneOfDiscriminator res;

    res.foo = value["foo"]
                  .template As<std::optional<USERVER_NAMESPACE::chaotic::OneOfWithDiscriminator<
                      &::ns::OneOfDiscriminator::kFoo_Settings,
                      USERVER_NAMESPACE::chaotic::Primitive<::ns::A>,
                      USERVER_NAMESPACE::chaotic::Primitive<::ns::B>>>>();

    USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value, k__ns__OneOfDiscriminator_PropertiesNames);

    return res;
}

}  // namespace ns

