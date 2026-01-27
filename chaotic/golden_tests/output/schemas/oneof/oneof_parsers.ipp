#pragma once

#include <userver/chaotic/object.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/variant.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

#include "oneof.hpp"

namespace ns {

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__OneOf_PropertiesNames = [](auto selector) {
    return selector().template Type<std::string_view>().Case("foo");
};

template <typename Value>
OneOf Parse(Value value, USERVER_NAMESPACE::formats::parse::To<OneOf>) {
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    OneOf res;

    res.foo =
        value["foo"]
            .template As<std::optional<USERVER_NAMESPACE::chaotic::Variant<
                USERVER_NAMESPACE::chaotic::Primitive<int>,
                USERVER_NAMESPACE::chaotic::Primitive<std::string>>>>();

    USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value, k__ns__OneOf_PropertiesNames);

    return res;
}

}  // namespace ns
