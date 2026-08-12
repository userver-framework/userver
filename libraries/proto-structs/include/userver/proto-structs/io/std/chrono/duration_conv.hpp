#pragma once

/// @file userver/proto-structs/io/std/chrono/duration_conv.hpp
/// @brief Provides read/write context class with the ability to handle `std::chrono::duration` conversion.

#include <userver/proto-structs/io/std/chrono/duration.hpp>

#include <google/protobuf/duration.pb.h>

#include <userver/proto-structs/duration.hpp>
#include <userver/proto-structs/exceptions.hpp>
#include <userver/proto-structs/io/context.hpp>
#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

template <typename TRep, typename TPeriod>
std::chrono::duration<TRep, TPeriod> ReadProtoStruct(
    ReadContext& ctx,
    To<std::chrono::duration<TRep, TPeriod>>,
    const ::google::protobuf::Duration& msg
) {
    using ChronoDuration = std::chrono::duration<TRep, TPeriod>;

    try {
        return Duration(::utils::impl::InternalTag{}, msg.seconds(), msg.nanos()).ToChronoDuration<ChronoDuration>();
    } catch (const ValueError& e) {
        ctx.AddError(e.what());
        return std::chrono::duration<TRep, TPeriod>{0};
    }
}

template <typename TRep, typename TPeriod>
void WriteProtoStruct(
    WriteContext& ctx,
    const std::chrono::duration<TRep, TPeriod>& obj,
    ::google::protobuf::Duration& msg
) {
    try {
        Duration duration{obj};
        msg.set_seconds(duration.Seconds().count());
        msg.set_nanos(static_cast<std::int32_t>(duration.Nanos().count()));
    } catch (const ValueError& e) {
        ctx.AddError(e.what());
    }
}

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
