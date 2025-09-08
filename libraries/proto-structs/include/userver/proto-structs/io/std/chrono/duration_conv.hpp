#pragma once

/// @file userver/proto-structs/io/std/chrono/duration_conv.hpp
/// @brief Provides read/write context class with the ability to handle `std::chrono::duration` conversion

#include <userver/proto-structs/io/std/chrono/duration.hpp>

#include <google/protobuf/duration.pb.h>
#include <google/protobuf/util/time_util.h>

#include <userver/proto-structs/io/context.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

template <typename TRep, typename TPeriod>
std::chrono::duration<TRep, TPeriod>
ReadProtoStruct(ReadContext& ctx, To<std::chrono::duration<TRep, TPeriod>>, const ::google::protobuf::Duration& msg) {
    using Duration = std::chrono::duration<TRep, TPeriod>;
    constexpr std::int64_t kMaxSecondsInDuration =
        std::chrono::duration_cast<std::chrono::seconds>(Duration::max()).count();
    constexpr std::int64_t kMinSecondsInDuration =
        std::chrono::duration_cast<std::chrono::seconds>(Duration::min()).count();

    Duration result;

    if (::google::protobuf::util::TimeUtil::IsDurationValid(msg)) {
        if (msg.seconds() > kMaxSecondsInDuration - 1) {
            result = Duration::max();
        } else if (msg.seconds() < kMinSecondsInDuration + 1) {
            result = Duration::min();
        } else {
            result = std::chrono::duration_cast<Duration>(
                std::chrono::seconds(msg.seconds()) + std::chrono::nanoseconds(msg.nanos())
            );
        }
    } else {
        ctx.AddError("invalid 'google.protobuf.Duration' value");
    }

    return result;
}

template <typename TRep, typename TPeriod>
void WriteProtoStruct(
    WriteContext&,
    const std::chrono::duration<TRep, TPeriod>& obj,
    ::google::protobuf::Duration& msg
) {
    using TimeUtil = ::google::protobuf::util::TimeUtil;
    const std::chrono::seconds seconds = std::chrono::duration_cast<std::chrono::seconds>(obj);
    const std::chrono::nanoseconds nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(obj - seconds);

    if (seconds.count() > TimeUtil::kDurationMaxSeconds) {
        msg.set_seconds(TimeUtil::kDurationMaxSeconds);
        msg.set_nanos(0);
    } else if (seconds.count() < TimeUtil::kDurationMinSeconds) {
        msg.set_seconds(TimeUtil::kDurationMinSeconds);
        msg.set_nanos(0);
    } else {
        msg.set_seconds(seconds.count());
        msg.set_nanos(nanos.count());
    }

    UASSERT(TimeUtil::IsDurationValid(msg));
}

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
