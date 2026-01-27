#pragma once

/// @file userver/proto-structs/io/userver/utils/strong_typedef_conv.hpp
/// @brief Provides read/write context class with the ability to handle `userver::utils::StrongTypedef` conversion.

#include <userver/proto-structs/io/userver/utils/strong_typedef.hpp>

#include <userver/proto-structs/io/context.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

template <typename TTag, typename TStructField, utils::StrongTypedefOps Ops, typename TEnable, typename TMessageField>
utils::StrongTypedef<TTag, TStructField, Ops, TEnable> ReadProtoField(
    ReadContext& ctx,
    To<utils::StrongTypedef<TTag, TStructField, Ops, TEnable>>,
    int field_number,
    const TMessageField& message_field
) {
    using Type = utils::StrongTypedef<TTag, TStructField, Ops, TEnable>;
    return Type{ctx.ReadField<TStructField>(field_number, message_field)};
}

template <typename TTag, typename TStructField, utils::StrongTypedefOps Ops, typename TEnable, typename TMessageField>
void WriteProtoField(
    WriteContext& ctx,
    const utils::StrongTypedef<TTag, TStructField, Ops, TEnable>& struct_field,
    int field_number,
    TMessageField& message_field
) {
    ctx.WriteField(struct_field.GetUnderlying(), field_number, message_field);
}

template <typename TTag, typename TStructField, utils::StrongTypedefOps Ops, typename TEnable, typename TMessageField>
void WriteProtoField(
    WriteContext& ctx,
    utils::StrongTypedef<TTag, TStructField, Ops, TEnable>&& struct_field,
    int field_number,
    TMessageField& message_field
) {
    ctx.WriteField(std::move(struct_field).GetUnderlying(), field_number, message_field);
}

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
