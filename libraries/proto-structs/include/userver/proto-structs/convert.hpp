#pragma once

/// @file userver/proto-structs/convert.hpp
/// @brief Functions for protobuf message to/from struct conversion.
///
/// Conversion functions rely on user-defined functions with the following signatures:
/// 1. `StructType ReadProtoStruct(proto_structs::io::ReadContext&,
///                                proto_structs::io::To<StructType>,
///                                const MessageType&)`
///
/// 2. `void WriteProtoStruct(proto_structs::io::WriteContext&,
///                           const StructType&,
///                           MessageType&)`
///
/// This functions should be foundable by ADL lookup (just place them in the namespace where `StructType` is defined).
///
/// It is recommended to define `WriteProtoStruct` function as template with a `StructType` parameter and use universal
/// reference `StructType&&` as a second parameter to enable perfect forwarding for conversions.

#include <type_traits>
#include <utility>

#include <userver/proto-structs/io/context.hpp>
#include <userver/proto-structs/type_mapping.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs {

/// @brief Converts protobuf message @a msg to proto struct @a obj.
/// @tparam TMessage protobuf message type
/// @tparam TStruct proto struct type
/// @throws ReadError if conversion has failed.
/// @warning If function throws an exception, @a obj is left in a valid but unspecified state.
template <traits::ProtoMessage TMessage, traits::ProtoStruct TStruct>
void MessageToStruct(const TMessage& msg, TStruct& obj) {
    io::ReadContext ctx(*msg.GetDescriptor());

    try {
        obj = ReadProtoStruct(ctx, io::To<std::remove_cv_t<TStruct>>{}, msg);
    } catch (const ReadError&) {
        // simply re-throw proper errors
        throw;
    } catch (const std::exception& e) {
        // some user-defined `ReadProtoStruct` threw an exception instead of adding an error to context,
        // adding it manually an re-throwing as a proper exception type
        ctx.AddError(e.what());
    }
}

/// @brief Converts protobuf message @a msg to specified proto struct type.
/// @tparam TStruct proto struct type
/// @tparam TMessage protobuf message type
/// @throws ReadError if conversion has failed.
template <traits::ProtoStruct TStruct, traits::ProtoMessage TMessage>
[[nodiscard]] TStruct MessageToStruct(const TMessage& msg) {
    TStruct obj;
    MessageToStruct(msg, obj);
    return obj;
}

/// @brief Converts proto struct @a obj to protobuf message @a msg.
/// @tparam TStruct proto struct type
/// @tparam TMessage protobuf message type
/// @throws WriteError if conversion has failed.
/// @warning If function throws an exception, @a msg is left in a valid but unspecified state.
template <typename TStruct, traits::ProtoMessage TMessage>
requires traits::ProtoStruct<std::remove_cvref_t<TStruct>>
void StructToMessage(TStruct&& obj, TMessage& msg) {
    io::WriteContext ctx(*msg.GetDescriptor());

    try {
        WriteProtoStruct(ctx, std::forward<TStruct>(obj), msg);
    } catch (const WriteError&) {
        // simply re-throw proper errors
        throw;
    } catch (const std::exception& e) {
        // some user-defined `WriteProtoStruct` threw an exception instead of adding an error to context,
        // adding it manually an re-throwing as a proper exception type
        ctx.AddError(e.what());
    }
}

/// @brief Converts proto struct @a obj to it's compatible protobuf message type.
/// @tparam TStruct proto struct type
/// @throws WriteError if conversion has failed.
template <typename TStruct>
requires traits::ProtoStruct<std::remove_cvref_t<TStruct>>
[[nodiscard]] traits::CompatibleMessageType<std::remove_cvref_t<TStruct>> StructToMessage(TStruct&& obj) {
    using Message = traits::CompatibleMessageType<std::remove_cvref_t<TStruct>>;
    Message msg;
    StructToMessage(std::forward<TStruct>(obj), msg);
    return msg;
}

}  // namespace proto_structs

USERVER_NAMESPACE_END
