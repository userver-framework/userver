#include <userver/protobuf/datetime.hpp>

USERVER_NAMESPACE_BEGIN

namespace protobuf {

bool IsValidDuration(const std::int64_t seconds, const std::int32_t nanos) noexcept {
    if (seconds < kMinDurationSeconds || seconds > kMaxDurationSeconds) {
        return false;
    }

    if (nanos < kMinDurationNanos || nanos > kMaxDurationNanos) {
        return false;
    }

    if ((seconds > 0 && nanos < 0) || (seconds < 0 && nanos > 0)) {
        return false;
    }

    return true;
}

bool IsValidTimestamp(const std::int64_t seconds, const std::int32_t nanos) noexcept {
    if (seconds < kMinTimestampSeconds || seconds > kMaxTimestampSeconds) {
        return false;
    }

    if (nanos < kMinTimestampNanos || nanos > kMaxTimestampNanos) {
        return false;
    }

    return true;
}

}  // namespace protobuf

USERVER_NAMESPACE_END
