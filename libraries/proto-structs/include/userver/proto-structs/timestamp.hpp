#pragma once

/// @file userver/proto-structs/timestamp.hpp
/// @brief @copybrief proto_structs::Timestamp

#include <chrono>
#include <cstdint>

#include <userver/proto-structs/duration.hpp>
#include <userver/proto-structs/impl/chrono_utils.hpp>
#include <userver/utils/impl/internal_tag_fwd.hpp>

namespace google::protobuf {
class Timestamp;
}  // namespace google::protobuf

USERVER_NAMESPACE_BEGIN

namespace proto_structs {

/// @brief Type to represent `google.protobuf.Timestamp` in proto structs.
///
/// This type is organized in the same way as its protobuf counterpart and allows for the same range of values
/// desribed here https://github.com/protocolbuffers/protobuf/blob/main/src/google/protobuf/timestamp.proto :
/// from `0001-01-01 00:00:00` to `9999-12-31 23:59:59` expressed as an offset from Unix epoch `1970-01-01 00:00:00`.
class Timestamp {
public:
    using ProtobufMessage = ::google::protobuf::Timestamp;
    using TimePoint = std::chrono::time_point<std::chrono::system_clock>;

    /// @brief Minimum allowed number of seconds.
    /// @note Represents `0001-01-01 00:00:00`.
    static constexpr std::chrono::seconds kMinSeconds{-62'135'596'800LL};

    /// @brief Maximum allowed number of seconds.
    /// @note Represents `9999-12-31 23:59:59`.
    static constexpr std::chrono::seconds kMaxSeconds{253'402'300'799LL};

    /// @brief Create zero (UTC epoch) timestamp.
    constexpr Timestamp() = default;

    /// @brief Creates timestamp from @a seconds specifying the offset from Unix epoch and @a nanos which are the
    ///        non-negative (from `0` to `999'999'999`) fractions of second.
    /// @throws ValueError if @a seconds and @a nanos does not satisty the requirements or outside the allowed range.
    /// @warning Note that @ref proto_structs::Duration treats @a nanos in a different way than `Timestamp`, which
    ///          means it is not correct in general to create timestamp from duration's `Seconds` and `Nanos` - use
    ///          specific constructor instead.
    constexpr Timestamp(const std::chrono::seconds& seconds, const std::chrono::nanoseconds& nanos)
        : seconds_(seconds),
          nanos_(nanos)
    {
        if (!IsValid(seconds_, nanos_)) {
            ThrowError(seconds_, nanos_);
        }
    }

    /// @brief Creates timestamp from duration @a since_epoch .
    /// @throws ValueError if @a since_epoch is outside the allowed range.
    constexpr explicit Timestamp(const Duration& since_epoch)
        : seconds_(since_epoch.Seconds()),
          nanos_(since_epoch.Nanos())
    {
        DurationToTimestamp(seconds_, nanos_);

        if (seconds_ < kMinSeconds || seconds_ > kMaxSeconds) {
            ThrowError(seconds_, nanos_);
        }
    }

    /// @brief Creates timestamp from `std::chrono::duration` @a since_epoch.
    /// @throws ValueError if @a since_epoch is outside the allowed range.
    template <typename TRep, typename TPeriod>
    constexpr explicit Timestamp(const std::chrono::duration<TRep, TPeriod>& since_epoch) {
        if (!impl::CanCastToSeconds(since_epoch)) {
            ThrowError("Cast from 'std::chrono::duration' will overflow/underflow");
        }

        seconds_ = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
        nanos_ = std::chrono::duration_cast<std::chrono::nanoseconds>(since_epoch - seconds_);
        DurationToTimestamp(seconds_, nanos_);

        if (seconds_ < kMinSeconds || seconds_ > kMaxSeconds) {
            ThrowError(seconds_, nanos_);
        }
    }

    /// @brief Creates timestamp from `std::chrono::system_clock::time_point`.
    /// @throws ValueError if @a time_point is outside the allowed range.
    constexpr Timestamp(const TimePoint& time_point)
        : Timestamp(time_point.time_since_epoch())
    {}

    Timestamp(utils::impl::InternalTag, std::int64_t seconds, std::int32_t nanos);

    /// @brief Returns offset from Unix epoch in seconds.
    [[nodiscard]] constexpr const std::chrono::seconds& Seconds() const noexcept { return seconds_; }

    /// @brief Returns fractions of second (from `0` to `999'999'999`) of the Unix epoch offset.
    [[nodiscard]] constexpr const std::chrono::nanoseconds& Nanos() const noexcept { return nanos_; }

    /// @brief Converts timestamp to `std::chrono::system_clock::time_point`.
    /// @warning If stored value does not fit in the `std::chrono::system_clock::time_point` it is capped at
    ///          time point min/max value.
    [[nodiscard]] constexpr TimePoint ToTimePoint() const noexcept {
        if (FitsInChronoTimePoint()) {
            return TimePoint{std::chrono::duration_cast<TimePoint::duration>(seconds_ + nanos_)};
        } else {
            return seconds_.count() < 0 ? TimePoint::min() : TimePoint::max();
        }
    }

    /// @brief Returns `true` if timestamp value fits in to the `std::chrono::system_clock::time_point`.
    [[nodiscard]] constexpr bool FitsInChronoTimePoint() const noexcept {
        return GetTimeSinceEpoch().FitsInChronoDuration<TimePoint::duration>();
    }

    /// @brief Returns duration since epoch.
    [[nodiscard]] constexpr Duration GetTimeSinceEpoch() const noexcept {
        Duration result;

        if (seconds_.count() >= 0 || nanos_.count() == 0) {
            result = Duration{seconds_, nanos_};
        } else {
            // 'seconds' is less than 0 and 'nanos' is greater than zero, should convert
            // to negative 'nanos'
            result = Duration{seconds_ + std::chrono::seconds{1}, nanos_ - std::chrono::seconds{1}};
        }

        return result;
    }

    /// @brief Explicit conversion to `std::chrono::system_clock::time_point`.
    /// @warning If stored value does not fit in the `std::chrono::system_clock::time_point` it is capped at
    ///          `std::chrono::system_clock::time_point` min/max value.
    [[nodiscard]] constexpr operator TimePoint() const noexcept { return ToTimePoint(); }

    /// @brief Default three-way comparison operator.
    auto operator<=>(const Timestamp&) const = default;

    /// @brief Returns minimum allowed timestamp.
    /// @note Minimum timestamp may not fit in 'std::chrono::system_clock' time point type.
    [[nodiscard]] static constexpr Timestamp Min() noexcept {
        Timestamp result;
        result.seconds_ = kMinSeconds;
        result.nanos_ = std::chrono::nanoseconds{0};
        return result;
    }

    /// @brief Returns maximum allowed timestamp.
    /// @note Maximum timestamp does not fit in 'std::chrono::system_clock::time_point'.
    [[nodiscard]] static constexpr Timestamp Max() noexcept {
        Timestamp result;
        result.seconds_ = kMaxSeconds;
        result.nanos_ = std::chrono::nanoseconds{999'999'999};
        return result;
    }

    /// @brief Returns `true` if @a seconds and @a nanos represent a valid `google.protobuf.Timestamp` value.
    [[nodiscard]] static constexpr bool IsValid(
        const std::chrono::seconds& seconds,
        const std::chrono::nanoseconds& nanos
    ) {
        if (seconds < kMinSeconds || seconds > kMaxSeconds) {
            return false;
        }

        if (nanos.count() < 0 || nanos.count() > 999'999'999) {
            return false;
        }

        return true;
    }

private:
    [[noreturn]] static void ThrowError(const std::chrono::seconds& seconds, const std::chrono::nanoseconds& nanos);
    [[noreturn]] static void ThrowError(const char* reason);

    static constexpr void DurationToTimestamp(std::chrono::seconds& seconds, std::chrono::nanoseconds& nanos) {
        if (nanos.count() < 0) {
            seconds -= std::chrono::seconds{1};
            nanos += std::chrono::seconds{1};
        }
    }

    std::chrono::seconds seconds_{0};
    std::chrono::nanoseconds nanos_{0};
};

}  // namespace proto_structs

USERVER_NAMESPACE_END
