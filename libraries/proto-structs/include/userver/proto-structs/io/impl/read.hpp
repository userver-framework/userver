#pragma once

#include <cstddef>
#include <optional>
#include <utility>

#include <userver/proto-structs/io/context.hpp>
#include <userver/proto-structs/io/impl/field_accessor.hpp>
#include <userver/proto-structs/type_mapping.hpp>
#include <userver/utils/meta_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io::impl {

template <typename TField, proto_structs::traits::ProtoMessage TMessage, typename TReturn>
TField ReadFieldWithGetter(
    proto_structs::io::ReadContext& ctx,
    proto_structs::io::To<TField>,
    const FieldGetter<TMessage, TReturn>& getter
) {
    static_assert(
        !meta::kIsCvInstantiationOf<std::optional, TField>,
        "'FieldGetterWithPresence' should be used for optional field"
    );
    return ctx.ReadField<TField>(getter.GetFieldNumber(), getter.GetValue());
}

template <typename TField, proto_structs::traits::ProtoMessage TMessage, typename TReturn>
std::optional<TField> ReadFieldWithGetter(
    proto_structs::io::ReadContext& ctx,
    proto_structs::io::To<std::optional<TField>>,
    const FieldGetterWithPresence<TMessage, TReturn>& getter
) {
    std::optional<TField> result;

    if (getter.HasValue()) {
        result = impl::ReadFieldWithGetter(
            ctx,
            proto_structs::io::To<TField>{},
            static_cast<const FieldGetter<TMessage, TReturn>&>(getter)
        );
    }

    return result;
}

template <std::size_t Index, typename TOneof, typename TMessage, typename TReturn>
bool ReadOneofFieldWithGetter(
    proto_structs::io::ReadContext& ctx,
    TOneof& oneof,
    const FieldGetterWithPresence<TMessage, TReturn>& getter
) {
    if (getter.HasValue()) {
        oneof.template Set<Index>(impl::ReadFieldWithGetter(
            ctx,
            To<proto_structs::OneofAlternativeType<Index, TOneof>>{},
            static_cast<const FieldGetter<TMessage, TReturn>&>(getter)
        ));
        return true;
    } else {
        oneof.Clear(Index);
        return false;
    }
}

template <typename TOneof, std::size_t... Index, typename... TGetters>
TOneof ReadOneof(proto_structs::io::ReadContext& ctx, std::index_sequence<Index...>, const TGetters&... getters) {
    TOneof oneof;
    (impl::ReadOneofFieldWithGetter<Index>(ctx, oneof, getters) || ...);
    return oneof;
}

template <typename TField, typename TGetter>
requires(!proto_structs::traits::Oneof<TField>) && traits::FieldGetter<TGetter>
TField ReadField(proto_structs::io::ReadContext& ctx, const TGetter& getter) {
    return impl::ReadFieldWithGetter(ctx, proto_structs::io::To<TField>{}, getter);
}

template <typename TField, typename... TGetters>
requires proto_structs::traits::Oneof<TField> && (traits::FieldGetterWithPresence<TGetters> && ...)
TField ReadField(proto_structs::io::ReadContext& ctx, const TGetters&... getters) {
    static_assert(TField::kSize == sizeof...(TGetters), "Field getter is not provided for some 'Oneof' field(s)");
    return impl::ReadOneof<TField>(ctx, std::make_index_sequence<sizeof...(TGetters)>{}, getters...);
}

}  // namespace proto_structs::io::impl

USERVER_NAMESPACE_END
