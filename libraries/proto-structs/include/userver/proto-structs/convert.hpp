#pragma once

/// @file userver/proto-structs/convert.hpp
/// @brief Functions for protobuf message to/from struct conversion

#include <type_traits>
#include <utility>

#include <userver/proto-structs/exceptions.hpp>
#include <userver/proto-structs/impl/context.hpp>
#include <userver/proto-structs/type_mapping.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs {

/// @brief Converts protobuf message @a msg to struct @a obj
/// @tparam TMessage protobuf message type
/// @tparam TStruct struct type
/// @throws ConversionError if conversion failed (in that case `obj` is not modified)
/// @note If function throws an exception, @a obj is not modified
///
/// Function calls `ReadStruct(obj, msg)` under the hood which should be foundable by argument-dependent lookup.
template <traits::ProtoMessage TMessage, traits::ProtoStruct TStruct>
void MessageToStruct(const TMessage& msg, TStruct& obj) {
    impl::ReadContext ctx;
    obj = ReadStruct(ctx, To<std::remove_cv_t<TStruct>>{}, msg);
    UINVARIANT(!ctx.HasError(), "Read context contains error that should have been thrown");
}

/// @brief Converts protobuf message @a msg to specified structure type
/// @tparam TStruct struct type
/// @tparam TMessage protobuf message type
/// @throws ConversionError if conversion failed
///
/// Function calls `ReadStruct(obj, msg)` under the hood which should be foundable by argument-dependent lookup.
template <traits::ProtoStruct TStruct, traits::ProtoMessage TMessage>
[[nodiscard]] TStruct MessageToStruct(const TMessage& msg) {
    TStruct obj;
    MessageToStruct(msg, obj);
    return obj;
}

/// @brief Converts struct instance @a obj to protobuf message @a msg
/// @tparam TStruct struct type
/// @tparam TMessage protobuf message type
/// @throws ConversionError if conversion failed (in that case `msg` is not modified)
/// @warning If function throws an exception, @a msg is left in a valid but unspecified state
///
/// Function calls `WriteStruct(std::forward<TStruct>(obj), msg)` under the hood which should be foundable by
/// argument-dependent lookup.
template <typename TStruct, traits::ProtoMessage TMessage>
requires traits::ProtoStruct<std::remove_cvref_t<TStruct>>
void StructToMessage(TStruct&& obj, TMessage& msg) {
    impl::WriteContext ctx;
    WriteStruct(ctx, std::forward<TStruct>(obj), msg);
    UINVARIANT(!ctx.HasError(), "Write context contains error that should have been thrown");
}

/// @brief Converts struct instance @a obj to protobuf message @a msg of the specified type
/// @tparam TStruct struct type
/// @tparam TMessage protobuf message type
/// @throws ConversionError if conversion failed
///
/// Function calls `WriteStruct(std::forward<TStruct>(obj), msg)` under the hood which should be foundable by
/// argument-dependent lookup.
template <traits::ProtoMessage TMessage, typename TStruct>
requires traits::ProtoStruct<std::remove_cvref_t<TStruct>>
[[nodiscard]] TMessage StructToMessage(TStruct&& obj) {
    TMessage msg;
    StructToMessage(std::forward<TStruct>(obj), msg);
    return msg;
}

/// @brief Converts struct instance @a obj to it's compatible protobuf message
/// @tparam TStruct struct type
/// @throws ConversionError if conversion failed
///
/// Compatability information should be provided for `TStruct` with @ref proto_structs::compatible_message, otherwise
/// compilation will fail.
///
/// Function calls `WriteStruct(std::forward<TStruct>(obj), msg)` under the hood which should be foundable by
/// argument-dependent lookup.
template <typename TStruct>
requires traits::CompatibleStruct<std::remove_cvref_t<TStruct>>
[[nodiscard]] traits::CompatibleMessageType<std::remove_cvref_t<TStruct>> StructToMessage(TStruct&& obj) {
    using Message = traits::CompatibleMessageType<std::remove_cvref_t<TStruct>>;
    return StructToMessage<Message>(std::forward<TStruct>(obj));
}

}  // namespace proto_structs

USERVER_NAMESPACE_END
