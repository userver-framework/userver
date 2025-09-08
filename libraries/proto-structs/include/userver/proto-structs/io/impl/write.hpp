#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

#include <userver/proto-structs/io/context.hpp>
#include <userver/proto-structs/io/impl/field_accessor.hpp>
#include <userver/proto-structs/type_mapping.hpp>
#include <userver/utils/meta_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io::impl {

template <typename TField, proto_structs::traits::ProtoMessage TMessage, typename TArg>
void WriteFieldWithSetter(
    proto_structs::io::WriteContext& ctx,
    TField&& value,
    const FieldSetterWithArg<TMessage, TArg>& setter
) {
    using ProtobufType = std::remove_cvref_t<TArg>;
    ProtobufType message_field{};
    ctx.WriteField(std::forward<TField>(value), setter.GetFieldNumber(), message_field);
    setter.SetValue(std::move(message_field));
}

template <typename TField, proto_structs::traits::ProtoMessage TMessage>
void WriteFieldWithSetter(
    proto_structs::io::WriteContext& ctx,
    TField&& value,
    const FieldSetterString<TMessage>& setter
) {
    std::string message_field{};
    ctx.WriteField(std::forward<TField>(value), setter.GetFieldNumber(), message_field);
    setter.SetValue(std::move(message_field));
}

template <typename TField, proto_structs::traits::ProtoMessage TMessage, typename TReturn>
void WriteFieldWithSetter(
    proto_structs::io::WriteContext& ctx,
    TField&& value,
    const FieldSetterWithMutable<TMessage, TReturn>& setter
) {
    ctx.WriteField(std::forward<TField>(value), setter.GetFieldNumber(), *setter.GetMutableValue());
}

template <std::size_t Index, typename TOneof, typename TSetter>
bool WriteOneofFieldWithSetter(proto_structs::io::WriteContext& ctx, TOneof&& oneof, const TSetter& setter) {
    if (oneof.Contains(Index)) {
        WriteFieldWithSetter(ctx, std::forward<TOneof>(oneof).template Get<Index>(), setter);
        return true;
    } else {
        setter.ClearValue();
        return false;
    }
}

template <typename TOneof, std::size_t... Index, typename... TSetters>
void WriteOneof(
    proto_structs::io::WriteContext& ctx,
    TOneof&& oneof,
    std::index_sequence<Index...>,
    const TSetters&... setters
) {
    (impl::WriteOneofFieldWithSetter<Index>(ctx, std::forward<TOneof>(oneof), setters) || ...);
}

template <typename TField, typename TSetter>
requires(!proto_structs::traits::Oneof<std::remove_cvref_t<TField>>) && traits::FieldSetter<TSetter>
void WriteField(proto_structs::io::WriteContext& ctx, TField&& value, const TSetter& setter) {
    using FieldType = std::remove_cvref_t<TField>;

    if constexpr (meta::kIsCvInstantiationOf<std::optional, FieldType>) {
        if (value) {
            impl::WriteFieldWithSetter(ctx, std::forward<TField>(value).value(), setter);
        } else {
            setter.ClearValue();
        }
    } else {
        impl::WriteFieldWithSetter(ctx, std::forward<TField>(value), setter);
    }
}

template <typename TField, typename... TSetters>
requires proto_structs::traits::Oneof<std::remove_cvref_t<TField>> && (traits::FieldSetter<TSetters> && ...)
void WriteField(proto_structs::io::WriteContext& ctx, TField&& value, const TSetters&... setters) {
    using OneofType = std::remove_cvref_t<TField>;
    static_assert(OneofType::kSize == sizeof...(TSetters), "Field setter is not provided for some 'Oneof' field(s)");
    impl::WriteOneof(ctx, std::forward<TField>(value), std::make_index_sequence<sizeof...(TSetters)>{}, setters...);
}

}  // namespace proto_structs::io::impl

USERVER_NAMESPACE_END
