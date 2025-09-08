#include <userver/proto-structs/io/std/chrono/time_point.hpp>

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/util/time_util.h>

#include <userver/proto-structs/io/context.hpp>
#include <userver/utils/assert.hpp>

namespace proto_structs::io {

std::chrono::time_point<std::chrono::system_clock> ReadProtoStruct(
    ReadContext& ctx,
    To<std::chrono::time_point<std::chrono::system_clock>>,
    const ::google::protobuf::Timestamp& msg
) {
    using TimePoint = std::chrono::time_point<std::chrono::system_clock>;
    constexpr std::int64_t kMaxSecondsInTimePoint =
        std::chrono::duration_cast<std::chrono::seconds>(TimePoint::duration::max()).count();
    constexpr std::int64_t kMinSecondsInTimePoint =
        std::chrono::duration_cast<std::chrono::seconds>(TimePoint::duration::min()).count();

    TimePoint result;

    if (::google::protobuf::util::TimeUtil::IsTimestampValid(msg)) {
        if (msg.seconds() > kMaxSecondsInTimePoint - 1) {
            result = TimePoint::max();
        } else if (msg.seconds() < kMinSecondsInTimePoint) {
            result = TimePoint::min();
        } else {
            result = TimePoint{std::chrono::duration_cast<TimePoint::duration>(
                std::chrono::seconds(msg.seconds()) + std::chrono::nanoseconds(msg.nanos())
            )};
        }
    } else {
        ctx.AddError("invalid 'google.protobuf.Timestamp' value");
    }

    return result;
}

void WriteProtoStruct(
    WriteContext&,
    const std::chrono::time_point<std::chrono::system_clock>& obj,
    ::google::protobuf::Timestamp& msg
) {
    using TimeUtil = ::google::protobuf::util::TimeUtil;
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(obj.time_since_epoch());
    const std::chrono::nanoseconds nanos = obj.time_since_epoch() - seconds;

    if (seconds.count() > TimeUtil::kTimestampMaxSeconds) {
        msg.set_seconds(TimeUtil::kTimestampMaxSeconds);
        msg.set_nanos(0);
    } else if (seconds.count() < TimeUtil::kTimestampMinSeconds) {
        msg.set_seconds(TimeUtil::kTimestampMinSeconds);
        msg.set_nanos(0);
    } else if (nanos.count() >= 0) {
        msg.set_seconds(seconds.count());
        msg.set_nanos(nanos.count());
    } else {
        // Timestamp.nanos should be from [0, 999'999'999]
        msg.set_seconds(seconds.count() - 1);
        msg.set_nanos(nanos.count() + 1'000'000'000);
    }

    UASSERT(TimeUtil::IsTimestampValid(msg));
}

}  // namespace proto_structs::io
