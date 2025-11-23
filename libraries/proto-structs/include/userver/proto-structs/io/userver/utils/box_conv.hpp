#pragma once

/// @file userver/proto-structs/io/userver/utils/box_conv.hpp
/// @brief Provides read/write context class with the ability to handle `userver::utils::Box` conversion.

#include <userver/proto-structs/io/userver/utils/box.hpp>

#include <userver/proto-structs/io/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

template <typename T, proto_structs::traits::ProtoMessage TMessage>
utils::Box<T> ReadProtoStruct(ReadContext& ctx, To<utils::Box<T>>, const TMessage& msg) {
    return {ReadProtoStruct(ctx, To<T>{}, msg)};
}

template <typename T, proto_structs::traits::ProtoMessage TMessage>
void WriteProtoStruct(WriteContext& ctx, const utils::Box<T>& obj, TMessage& msg) {
    WriteProtoStruct(ctx, *obj, msg);
}

template <typename T, proto_structs::traits::ProtoMessage TMessage>
void WriteProtoStruct(WriteContext& ctx, utils::Box<T>&& obj, TMessage& msg) {
    WriteProtoStruct(ctx, std::move(*obj), msg);
}

template <typename TStructField, typename TMessageField>
utils::Box<TStructField> ReadProtoField(
    ReadContext& ctx,
    To<utils::Box<TStructField>>,
    int field_number,
    const TMessageField& message_field
) {
    return {ctx.ReadField<TStructField>(field_number, message_field)};
}

template <typename TStructField, typename TMessageField>
void WriteProtoField(
    WriteContext& ctx,
    const utils::Box<TStructField>& struct_field,
    int field_number,
    TMessageField& message_field
) {
    ctx.WriteField(*struct_field, field_number, message_field);
}

template <typename TStructField, typename TMessageField>
void WriteProtoField(
    WriteContext& ctx,
    utils::Box<TStructField>&& struct_field,
    int field_number,
    TMessageField& message_field
) {
    ctx.WriteField(std::move(*struct_field), field_number, message_field);
}

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
