#pragma once

#include <type_traits>

#include <google/protobuf/map.h>

#include <userver/proto-structs/io/context.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io::impl {

template <typename TMap, typename TKey, typename TValue>
TMap ReadMap(ReadContext& ctx, int field_number, const ::google::protobuf::Map<TKey, TValue>& message_field) {
    using Key = typename TMap::key_type;
    using Value = typename TMap::mapped_type;
    TMap result;

    for (const auto& [key, val] : message_field) {
        result.emplace(ctx.ReadField<Key>(field_number, key), ctx.ReadField<Value>(field_number, val));
    }

    return result;
}

template <typename TMap, typename TKey, typename TValue>
void WriteMap(
    WriteContext& ctx,
    TMap&& struct_field,
    int field_number,
    ::google::protobuf::Map<TKey, TValue>& message_field
) {
    message_field.clear();

    for (auto& [key, val] : struct_field) {
        TKey msg_key;
        ctx.WriteField(key, field_number, msg_key);

        if constexpr (std::is_rvalue_reference_v<decltype(struct_field)>) {
            ctx.WriteField(std::move(val), field_number, message_field[std::move(msg_key)]);
        } else {
            ctx.WriteField(val, field_number, message_field[std::move(msg_key)]);
        }
    }
}

}  // namespace proto_structs::io::impl

USERVER_NAMESPACE_END
