#pragma once

#include <string>

#include <userver/chaotic/exception.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/utils/string_literal.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic {

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
    typename ModeDescriptor::Value(StructType::*FieldMemberPtr),
    const utils::StringLiteral& RawName>
struct Field final {
    using Struct = StructType;
    using ModeDescriptorType = ModeDescriptor;
    using Descriptor = typename ModeDescriptor::Descriptor;
    using FieldType = typename ModeDescriptor::Value;
    static constexpr const utils::StringLiteral& Name = RawName;
    static constexpr FieldType StructType::*kField = FieldMemberPtr;
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
