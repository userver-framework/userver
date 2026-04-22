#pragma once

/// @file userver/proto-structs/io/std/vector_conv.hpp
/// @brief Provides read/write context class with the ability to handle `std::vector` conversion.

#include <userver/proto-structs/io/std/vector.hpp>

#include <type_traits>

#include <google/protobuf/repeated_field.h>
#include <google/protobuf/repeated_ptr_field.h>

#include <userver/proto-structs/io/context.hpp>
#include <userver/proto-structs/type_mapping.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

namespace impl {

template <typename TVector, proto_structs::traits::ProtoRepeated TRepeated>
void WriteVector(WriteContext& ctx, TVector&& struct_field, int field_number, TRepeated& message_field) {
    message_field.Clear();
    message_field.Reserve(static_cast<int>(struct_field.size()));

    for (auto& item : struct_field) {
        if constexpr (std::is_rvalue_reference_v<decltype(struct_field)>) {
            ctx.WriteField(std::move(item), field_number, *message_field.Add());
        } else {
            ctx.WriteField(item, field_number, *message_field.Add());
        }
    }
}

}  // namespace impl

template <typename TItem, typename TAllocator, proto_structs::traits::ProtoRepeated TRepeated>
std::vector<TItem, TAllocator> ReadProtoField(
    ReadContext& ctx,
    To<std::vector<TItem, TAllocator>>,
    int field_number,
    const TRepeated& message_field
) {
    std::vector<TItem, TAllocator> result;
    result.reserve(message_field.size());

    for (const auto& item : message_field) {
        result.push_back(ctx.ReadField<TItem>(field_number, item));
    }

    return result;
}

template <typename TItem, typename TAllocator, proto_structs::traits::ProtoRepeated TRepeated>
void WriteProtoField(
    WriteContext& ctx,
    const std::vector<TItem, TAllocator>& struct_field,
    int field_number,
    TRepeated& message_field
) {
    impl::WriteVector(ctx, struct_field, field_number, message_field);
}

template <typename TItem, typename TAllocator, proto_structs::traits::ProtoRepeated TRepeated>
void WriteProtoField(
    WriteContext& ctx,
    std::vector<TItem, TAllocator>&& struct_field,
    int field_number,
    TRepeated& message_field
) {
    impl::WriteVector(ctx, std::move(struct_field), field_number, message_field);
}

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
