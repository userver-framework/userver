#pragma once

#include <chrono>
#include <limits>
#include <map>
#include <optional>
#include <type_traits>
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
#include <userver/utils/assert.hpp>
#include <userver/utils/box.hpp>
#include <userver/utils/datetime/date.hpp>
#include <userver/utils/time_of_day.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::impl {

template <typename TStructField, typename TMessageField>
void ToProtobuf(WriteContext&, const FieldAccessor&, TStructField&&, TMessageField&) {
    static_assert(sizeof(TStructField) && false, "No known conversion from struct field type to protobuf field type");
}

template <typename TStructField, typename TMessageField>
requires std::is_same_v<std::remove_cvref_t<TStructField>, std::remove_cv_t<TMessageField>> &&
         traits::ProtoScalar<TMessageField>
TStructField ToProtobuf(WriteContext&, const FieldAccessor&, TStructField&& struct_field, TMessageField& msg_field) {
    msg_field = std::forward<TStructField>(struct_field);
}

void ToProtobuf(
    WriteContext&,
    const FieldAccessor&,
    const std::chrono::time_point<std::chrono::system_clock>& struct_field,
    ::google::protobuf::Timestamp& msg_field
) {
    using TimeUtil = ::google::protobuf::util::TimeUtil;
    const std::chrono::seconds seconds =
        std::chrono::duration_cast<std::chrono::seconds>(struct_field.time_since_epoch());
    const std::chrono::nanoseconds nanos = struct_field.time_since_epoch() - seconds;

    if (seconds.count() > TimeUtil::kTimestampMaxSeconds) {
        msg_field.set_seconds(TimeUtil::kTimestampMaxSeconds);
        msg_field.set_nanos(0);
    } else if (seconds.count() < TimeUtil::kTimestampMinSeconds) {
        msg_field.set_seconds(TimeUtil::kTimestampMinSeconds);
        msg_field.set_nanos(0);
    } else if (nanos.count() >= 0) {
        msg_field.set_seconds(seconds.count());
        msg_field.set_nanos(nanos.count());
    } else {
        // Timestamp.nanos should be from [0, 999'999'999]
        msg_field.set_seconds(seconds.count() - 1);
        msg_field.set_nanos(nanos.count() + 1);
    }

    UASSERT(TimeUtil::IsTimestampValid(msg_field));
}

template <typename TRep, typename TPeriod>
void ToProtobuf(
    WriteContext&,
    const FieldAccessor&,
    const std::chrono::duration<TRep, TPeriod>& struct_field,
    ::google::protobuf::Duration& msg_field
) {
    using TimeUtil = ::google::protobuf::util::TimeUtil;
    const std::chrono::seconds seconds = std::chrono::duration_cast<std::chrono::seconds>(struct_field);
    const std::chrono::nanoseconds nanos = struct_field - seconds;

    if (seconds.count() > TimeUtil::kDurationMaxSeconds) {
        msg_field.set_seconds(TimeUtil::kDurationMaxSeconds);
        msg_field.set_nanos(0);
    } else if (seconds.count() < TimeUtil::kDurationMinSeconds) {
        msg_field.set_seconds(TimeUtil::kDurationMaxSeconds);
        msg_field.set_nanos(0);
    } else {
        msg_field.set_seconds(seconds.count());
        msg_field.set_nanos(nanos.count());
    }

    UASSERT(TimeUtil::IsDurationValid(msg_field));
}

template <typename TStruct, proto_structs::traits::ProtoMessage TMessage>
requires proto_structs::traits::ProtoStruct<std::remove_reference_t<TStruct>>
void ToProtobuf(WriteContext& ctx, const FieldAccessor&, TStruct&& struct_field, TMessage& msg_field) {
    WriteStruct(ctx, std::forward<TStruct>(struct_field), msg_field);
}

template <typename TVector, typename TItem>
requires traits::Vector<std::remove_reference_t<TVector>>
void ToProtobuf(
    WriteContext& ctx,
    const FieldAccessor& accessor,
    TVector&& struct_field,
    ::google::protobuf::RepeatedPtrField<TItem>& msg_field
) {
    msg_field.Clear();
    msg_field.Reserve(static_cast<int>(struct_field.size()));

    for (auto& item : struct_field) {
        if constexpr (std::is_rvalue_reference_v<decltype(struct_field)>) {
            impl::ToProtobuf(ctx, accessor, std::move(item), *msg_field.Add());
        } else {
            impl::ToProtobuf(ctx, accessor, item, *msg_field.Add());
        }
    }
}

template <typename TMap, typename TKey, typename TValue>
requires traits::Map<std::remove_reference_t<TMap>>
void ToProtobuf(
    WriteContext& ctx,
    const FieldAccessor& accessor,
    TMap&& struct_field,
    ::google::protobuf::Map<TKey, TValue>& msg_field
) {
    msg_field.clear();

    for (auto& [key, val] : struct_field) {
        TKey msg_key;
        impl::ToProtobuf(ctx, accessor, key, msg_key);

        if constexpr (std::is_rvalue_reference_v<decltype(struct_field)>) {
            impl::ToProtobuf(ctx, accessor, std::move(val), msg_field[std::move(msg_key)]);
        } else {
            impl::ToProtobuf(ctx, accessor, val, msg_field[std::move(msg_key)]);
        }
    }
}

template <typename TField, proto_structs::traits::ProtoMessage TMessage, typename TArg>
void WriteFieldWithSetter(WriteContext& ctx, TField&& value, const FieldSetterWithArg<TMessage, TArg>& setter) {
    using ProtobufType = std::remove_cvref_t<TArg>;
    ProtobufType msg_field{};
    impl::ToProtobuf(ctx, setter, std::forward<TField>(value), msg_field);
    setter.SetValue(std::move(msg_field));
}

template <typename TField, proto_structs::traits::ProtoMessage TMessage, typename TReturn>
void WriteFieldWithSetter(WriteContext& ctx, TField&& value, const FieldSetterWithMutable<TMessage, TReturn>& setter) {
    impl::ToProtobuf(ctx, setter, std::forward<TField>(value), *setter.GetMutableValue());
}

template <std::size_t Index, typename TOneof, typename TSetter>
bool WriteOneofFieldWithSetter(WriteContext& ctx, TOneof&& oneof, const TSetter& setter) {
    if (oneof.Contains(Index)) {
        WriteFieldWithSetter(ctx, std::forward<TOneof>(oneof).template Get<Index>(), setter);
        return true;
    } else {
        setter.ClearValue();
        return false;
    }
}

template <typename TOneof, std::size_t... Index, typename... TSetters>
void WriteOneof(WriteContext& ctx, TOneof&& oneof, std::index_sequence<Index...>, const TSetters&... setters) {
    (impl::WriteOneofFieldWithSetter<Index>(ctx, std::forward<TOneof>(oneof), setters) || ...);
}

template <typename TField, typename TSetter>
requires(!proto_structs::traits::Oneof<std::remove_reference_t<TField>>) && traits::FieldSetter<TSetter>
void WriteField(WriteContext& ctx, TField&& value, const TSetter& setter) {
    if constexpr (traits::Optional<TField>) {
        if (value) {
            impl::WriteFieldWithSetter(ctx, std::forward<TField>(value).value(), setter);
        } else {
            setter.CleanValue();
        }
    } else {
        impl::WriteFieldWithSetter(ctx, std::forward<TField>(value), setter);
    }

    if (ctx.HasError()) {
        throw std::move(ctx).GetError();
    }
}

template <typename TField, typename... TSetters>
requires proto_structs::traits::Oneof<std::remove_reference_t<TField>> && (traits::FieldSetter<TSetters> && ...)
void WriteField(WriteContext& ctx, TField&& value, const TSetters&... setters) {
    static_assert(TField::kSize == sizeof...(TSetters), "Field setter is not provided for some 'Oneof' field(s)");

    impl::WriteOneof(ctx, std::forward<TField>(value), std::make_index_sequence<sizeof...(TSetters)>{}, setters...);

    if (ctx.HasError()) {
        throw std::move(ctx).GetError();
    }
}

}  // namespace proto_structs::impl

USERVER_NAMESPACE_END
