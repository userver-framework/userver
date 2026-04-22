#pragma once

#include <string>

#include <userver/chaotic/exception.hpp>
#include <userver/formats/common/items.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utils/string_literal.hpp>
#include <userver/utils/trivial_map.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace chaotic {

template <typename BuilderFunc, typename Value>
Value ExtractAdditionalPropertiesTrue(const Value& json, const utils::TrivialSet<BuilderFunc>& names_to_exclude) {
    typename Value::Builder builder(formats::common::Type::kObject);

    for (const auto& [name, value] : formats::common::Items(json)) {
        if (names_to_exclude.Contains(name)) {
            continue;
        }

        builder[name] = value;
    }
    return builder.ExtractValue();
}

template <typename BuilderFunc, typename Value>
void ValidateNoAdditionalProperties(const Value& json, const utils::TrivialSet<BuilderFunc>& names_to_exclude) {
    for (const auto& [name, value] : formats::common::Items(json)) {
        if (names_to_exclude.Contains(name)) {
            continue;
        }

        chaotic::ThrowForValue<Value>(fmt::format("Unknown property '{}'", name), value);
    }
}

template <typename T, template <typename...> typename Map, typename Value, typename BuilderFunc>
Map<std::string, formats::common::ParseType<Value, T>> ExtractAdditionalProperties(
    const Value& json,
    const utils::TrivialSet<BuilderFunc>& names_to_exclude
) {
    Map<std::string, formats::common::ParseType<Value, T>> map;

    for (const auto& [name, value] : formats::common::Items(json)) {
        if (names_to_exclude.Contains(name)) {
            continue;
        }

        map.emplace(name, value.template As<T>());
    }
    return map;
}

template <typename DescriptorType>
using TypeOfDescriptor = decltype(formats::json::Value{}.As<DescriptorType>());

template <typename DescriptorType>
struct Required {
    using Value = TypeOfDescriptor<DescriptorType>;
    using Descriptor = DescriptorType;
};

template <typename DescriptorType>
struct Optional {
    using Value = std::optional<TypeOfDescriptor<DescriptorType>>;
    using Descriptor = DescriptorType;
};

template <typename DescriptorType, typename T, const T& Default>
struct Defaulted {
    using Value = TypeOfDescriptor<DescriptorType>;
    using Descriptor = DescriptorType;

    static constexpr const T& DefaultValue = Default;
};

template <typename T>
struct IsDefaulted : std::false_type {};

template <typename DescriptorType, typename T, const T& Default>
struct IsDefaulted<Defaulted<DescriptorType, T, Default>> : std::true_type {};

template <
    typename StructType,
    typename ModeDescriptor,
    typename ModeDescriptor::Value(StructType::*FieldName),
    const utils::StringLiteral& RawName>
struct Field final {
    using Struct = StructType;
    using ModeDescriptorType = ModeDescriptor;
    using Descriptor = typename ModeDescriptor::Descriptor;
    using FieldType = typename ModeDescriptor::Value;
    static constexpr const utils::StringLiteral& Name = RawName;
    static constexpr FieldType StructType::*kField = FieldName;
};

struct UnknownFields {
    struct Ignore {};

    struct StoreJson {};

    struct Forbid {};

    template <typename T>
    struct StoreTyped {
        using Type = T;
    };
};

template <typename StructType, typename Unknown, typename... Fields>
struct Object final {};

}  // namespace chaotic

USERVER_NAMESPACE_END
