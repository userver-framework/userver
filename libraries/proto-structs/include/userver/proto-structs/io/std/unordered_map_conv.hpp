#pragma once

/// @file userver/proto-structs/io/std/unordered_map_conv.hpp
/// @brief Provides read/write context class with the ability to handle `std::unordered_map` conversion.

#include <userver/proto-structs/io/std/unordered_map.hpp>

#include <google/protobuf/map.h>

#include <userver/proto-structs/io/impl/std/any_map_conv.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

template <
    typename TKey,
    typename TValue,
    typename THash,
    typename TKeyEqual,
    typename TAllocator,
    typename TProtoKey,
    typename TProtoValue>
std::unordered_map<TKey, TValue, THash, TKeyEqual, TAllocator> ReadProtoField(
    ReadContext& ctx,
    To<std::unordered_map<TKey, TValue, THash, TKeyEqual, TAllocator>>,
    int field_number,
    const ::google::protobuf::Map<TProtoKey, TProtoValue>& message_field
) {
    return impl::ReadMap<
        std::unordered_map<TKey, TValue, THash, TKeyEqual, TAllocator>>(ctx, field_number, message_field);
}

template <
    typename TKey,
    typename TValue,
    typename THash,
    typename TKeyEqual,
    typename TAllocator,
    typename TProtoKey,
    typename TProtoValue>
void WriteProtoField(
    WriteContext& ctx,
    const std::unordered_map<TKey, TValue, THash, TKeyEqual, TAllocator>& struct_field,
    int field_number,
    ::google::protobuf::Map<TProtoKey, TProtoValue>& message_field
) {
    impl::WriteMap(ctx, struct_field, field_number, message_field);
}

template <
    typename TKey,
    typename TValue,
    typename THash,
    typename TKeyEqual,
    typename TAllocator,
    typename TProtoKey,
    typename TProtoValue>
void WriteProtoField(
    WriteContext& ctx,
    std::unordered_map<TKey, TValue, THash, TKeyEqual, TAllocator>&& struct_field,
    int field_number,
    ::google::protobuf::Map<TProtoKey, TProtoValue>& message_field
) {
    impl::WriteMap(ctx, std::move(struct_field), field_number, message_field);
}

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
