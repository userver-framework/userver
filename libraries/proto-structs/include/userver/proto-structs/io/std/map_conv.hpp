#pragma once

/// @file userver/proto-structs/io/std/map_conv.hpp
/// @brief Provides read/write context class with the ability to handle `std::map` conversion.

#include <userver/proto-structs/io/std/map.hpp>

#include <google/protobuf/map.h>

#include <userver/proto-structs/io/impl/std/any_map_conv.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

template <
    typename TKey,
    typename TValue,
    typename TCompare,
    typename TAllocator,
    typename TProtoKey,
    typename TProtoValue>
std::map<TKey, TValue, TCompare, TAllocator> ReadProtoField(
    ReadContext& ctx,
    To<std::map<TKey, TValue, TCompare, TAllocator>>,
    int field_number,
    const ::google::protobuf::Map<TProtoKey, TProtoValue>& message_field
) {
    return impl::ReadMap<std::map<TKey, TValue, TCompare, TAllocator>>(ctx, field_number, message_field);
}

template <
    typename TKey,
    typename TValue,
    typename TCompare,
    typename TAllocator,
    typename TProtoKey,
    typename TProtoValue>
void WriteProtoField(
    WriteContext& ctx,
    const std::map<TKey, TValue, TCompare, TAllocator>& struct_field,
    int field_number,
    ::google::protobuf::Map<TProtoKey, TProtoValue>& message_field
) {
    impl::WriteMap(ctx, struct_field, field_number, message_field);
}

template <
    typename TKey,
    typename TValue,
    typename TCompare,
    typename TAllocator,
    typename TProtoKey,
    typename TProtoValue>
void WriteProtoField(
    WriteContext& ctx,
    std::map<TKey, TValue, TCompare, TAllocator>&& struct_field,
    int field_number,
    ::google::protobuf::Map<TProtoKey, TProtoValue>& message_field
) {
    impl::WriteMap(ctx, std::move(struct_field), field_number, message_field);
}

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
