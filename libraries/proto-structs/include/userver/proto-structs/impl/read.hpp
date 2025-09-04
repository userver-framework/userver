#pragma once

#include <chrono>
#include <limits>
#include <map>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include <google/protobuf/duration.pb.h>
#include <google/protobuf/map.h>
#include <google/protobuf/repeated_ptr_field.h>
#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/util/time_util.h>

#include <userver/decimal64/decimal64.hpp>
#include <userver/proto-structs/impl/context.hpp>
#include <userver/proto-structs/impl/field.hpp>
#include <userver/proto-structs/impl/traits.hpp>
#include <userver/proto-structs/oneof.hpp>
#include <userver/proto-structs/type_mapping.hpp>
#include <userver/utils/box.hpp>
#include <userver/utils/datetime/date.hpp>
#include <userver/utils/time_of_day.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::impl {

template <typename TStructField, typename TMsgField>
TStructField FromProtobuf(ReadContext&, const FieldAccessor&, To<TStructField>, const TMsgField&) {
    static_assert(sizeof(TStructField) && false, "No known conversion from protobuf field type to struct field type");
    return TStructField{};
}

template <typename TField>
requires traits::ProtoScalar<TField>
TField FromProtobuf(ReadContext&, const FieldAccessor&, To<TField>, const TField& value) {
    return value;
}

std::chrono::time_point<std::chrono::system_clock> FromProtobuf(
    ReadContext& ctx,
    const FieldAccessor& accessor,
    To<std::chrono::time_point<std::chrono::system_clock>>,
    const ::google::protobuf::Timestamp& value
) {
    using TimePoint = std::chrono::time_point<std::chrono::system_clock>;
    constexpr std::int64_t kMaxSecondsInTimePoint =
        std::chrono::duration_cast<std::chrono::seconds>(TimePoint::duration::max()).count();
    constexpr std::int64_t kMinSecondsInTimePoint =
        std::chrono::duration_cast<std::chrono::seconds>(TimePoint::duration::min()).count();

    TimePoint result;

    if (::google::protobuf::util::TimeUtil::IsTimestampValid(value)) {
        if (value.seconds() > kMaxSecondsInTimePoint - 1) {
            result = TimePoint::max();
        } else if (value.seconds() < kMinSecondsInTimePoint) {
            result = TimePoint::min();
        } else {
            result = TimePoint{std::chrono::duration_cast<TimePoint::duration>(
                std::chrono::seconds(value.seconds()) + std::chrono::nanoseconds(value.nanos())
            )};
        }
    } else {
        ctx.SetError(accessor.GetFieldDescriptor(), "invalid 'google.protobuf.Timestamp' value");
    }

    return result;
}

template <typename TRep, typename TPeriod>
std::chrono::duration<TRep, TPeriod> FromProtobuf(
    ReadContext& ctx,
    const FieldAccessor& accessor,
    To<std::chrono::duration<TRep, TPeriod>>,
    const ::google::protobuf::Duration& value
) {
    using Duration = std::chrono::duration<TRep, TPeriod>;
    constexpr std::int64_t kMaxSecondsInDuration =
        std::chrono::duration_cast<std::chrono::seconds>(Duration::max()).count();
    constexpr std::int64_t kMinSecondsInDuration =
        std::chrono::duration_cast<std::chrono::seconds>(Duration::min()).count();

    Duration result;

    if (::google::protobuf::util::TimeUtil::IsDurationValid(value)) {
        if (value.seconds() > kMaxSecondsInDuration - 1) {
            result = Duration::max();
        } else if (value.seconds() < kMinSecondsInDuration + 1) {
            result = Duration::min();
        } else {
            result = std::chrono::duration_cast<Duration>(
                std::chrono::seconds(value.seconds()) + std::chrono::nanoseconds(value.nanos())
            );
        }
    } else {
        ctx.SetError(accessor.GetFieldDescriptor(), "invalid 'google.protobuf.Duration' value");
    }

    return result;
}

template <proto_structs::traits::ProtoStruct TField, proto_structs::traits::ProtoMessage TMessage>
TField FromProtobuf(ReadContext& ctx, const FieldAccessor&, To<TField> dst, const TMessage& value) {
    return ReadStruct(ctx, dst, value);
}

template <typename TItem, typename TAllocator>
std::vector<TItem, TAllocator> FromProtobuf(
    ReadContext& ctx,
    const FieldAccessor& accessor,
    To<std::vector<TItem, TAllocator>>,
    const ::google::protobuf::RepeatedPtrField<TItem>& value
) {
    std::vector<TItem, TAllocator> result;
    result.reserve(value.size());

    for (const auto& item : value) {
        result.push_back(impl::FromProtobuf(ctx, accessor, To<TItem>{}, item));
    }

    return result;
}

template <typename TMap, typename TKey, typename TValue>
requires traits::Map<TMap>
TMap FromProtobuf(
    ReadContext& ctx,
    const FieldAccessor& accessor,
    To<TMap>,
    const ::google::protobuf::Map<TKey, TValue>& value
) {
    using Key = typename TMap::key_type;
    using Value = typename TMap::mapped_type;
    TMap result;

    for (const auto& [key, val] : value) {
        result.emplace(
            impl::FromProtobuf(ctx, accessor, To<Key>{}, key), impl::FromProtobuf(ctx, accessor, To<Value>{}, val)
        );
    }

    return result;
}

template <typename TField, proto_structs::traits::ProtoMessage TMessage, typename TReturn>
TField ReadFieldWithGetter(ReadContext& ctx, To<TField> dst, const FieldGetter<TMessage, TReturn>& getter) {
    static_assert(!traits::Optional<TField>, "'FieldGetterWithPresence' should be used for optional field");
    return impl::FromProtobuf(ctx, getter, dst, getter.GetValue());
}

template <typename TField, proto_structs::traits::ProtoMessage TMessage, typename TReturn>
std::optional<TField> ReadFieldWithGetter(
    ReadContext& ctx,
    To<std::optional<TField>>,
    const FieldGetterWithPresence<TMessage, TReturn>& getter
) {
    static_assert(
        traits::ProtoScalar<TField> || proto_structs::traits::ProtoMessage<TField>, "Unsupported type for 'optional'"
    );
    std::optional<TField> result;

    if (getter.HasValue()) {
        result =
            impl::ReadFieldWithGetter(ctx, To<TField>{}, static_cast<const FieldGetter<TMessage, TReturn>&>(getter));
    }

    return result;
}

template <std::size_t Index, typename TOneof, typename TMessage, typename TReturn>
bool ReadOneofFieldWithGetter(
    ReadContext& ctx,
    TOneof& oneof,
    const FieldGetterWithPresence<TMessage, TReturn>& getter
) {
    if (getter.HasValue()) {
        oneof.Set<Index>(impl::ReadFieldWithGetter(
            ctx, To<OneofAlternativeType<Index, TOneof>>{}, static_cast<const FieldGetter<TMessage, TReturn>&>(getter)
        ));
        return true;
    } else {
        oneof.Clear(Index);
        return false;
    }
}

template <typename TOneof, std::size_t... Index, typename... TGetters>
TOneof ReadOneof(ReadContext& ctx, std::index_sequence<Index...>, const TGetters&... getters) {
    TOneof oneof;
    (impl::ReadOneofFieldWithGetter<Index>(ctx, oneof, getters) || ...);
    return oneof;
}

template <typename TField, typename TGetter>
requires(!proto_structs::traits::Oneof<TField>) && traits::FieldGetter<TGetter>
TField ReadField(ReadContext& ctx, const TGetter& getter) {
    auto result = impl::ReadFieldWithGetter(ctx, To<TField>{}, getter);

    if (ctx.HasError()) {
        throw std::move(ctx).GetError();
    }

    return result;
}

template <typename TField, typename... TGetters>
requires proto_structs::traits::Oneof<TField> && (traits::FieldGetterWithPresence<TGetters> && ...)
TField ReadField(ReadContext& ctx, const TGetters&... getters) {
    static_assert(TField::kSize == sizeof...(TGetters), "Field getter is not provided for some 'Oneof' field(s)");

    auto result = impl::ReadOneof<TField>(ctx, std::make_index_sequence<sizeof...(TGetters)>{}, getters...);

    if (ctx.HasError()) {
        throw std::move(ctx).GetError();
    }

    return result;
}

}  // namespace proto_structs::impl

USERVER_NAMESPACE_END
