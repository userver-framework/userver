#pragma once

#include <userver/chaotic/object.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/common/merge.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

#include "allof.hpp"

namespace ns {

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__AllOf__Foo__P0_PropertiesNames = [](auto selector) {
    return selector().template Type<std::string_view>().Case("foo");
};

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__AllOf__Foo__P1_PropertiesNames = [](auto selector) {
    return selector().template Type<std::string_view>().Case("bar");
};

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__AllOf_PropertiesNames = [](auto selector) {
    return selector().template Type<std::string_view>().Case("foo");
};

template <typename Value>
AllOf::Foo__P0 Parse(Value value, USERVER_NAMESPACE::formats::parse::To<AllOf::Foo__P0>) {
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    AllOf::Foo__P0 res;

    res.foo = value["foo"].template As<std::optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>>();

    res.extra =
        USERVER_NAMESPACE::chaotic::ExtractAdditionalPropertiesTrue(value, k__ns__AllOf__Foo__P0_PropertiesNames);

    return res;
}

template <typename Value>
AllOf::Foo__P1 Parse(Value value, USERVER_NAMESPACE::formats::parse::To<AllOf::Foo__P1>) {
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    AllOf::Foo__P1 res;

    res.bar = value["bar"].template As<std::optional<USERVER_NAMESPACE::chaotic::Primitive<int>>>();

    res.extra =
        USERVER_NAMESPACE::chaotic::ExtractAdditionalPropertiesTrue(value, k__ns__AllOf__Foo__P1_PropertiesNames);

    return res;
}

template <typename Value>
AllOf::Foo Parse(Value value, USERVER_NAMESPACE::formats::parse::To<AllOf::Foo>) {
    return AllOf::Foo(value.template As<AllOf::Foo__P0>(), value.template As<AllOf::Foo__P1>());
}

template <typename Value>
AllOf Parse(Value value, USERVER_NAMESPACE::formats::parse::To<AllOf>) {
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    AllOf res;

    res.foo = value["foo"].template As<std::optional<USERVER_NAMESPACE::chaotic::Primitive<::ns::AllOf::Foo>>>();

    USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value, k__ns__AllOf_PropertiesNames);

    return res;
}

}  // namespace ns
