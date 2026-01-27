#pragma once

/// @file userver/proto-structs/duration.hpp
/// @brief @copybrief proto_structs::Duration

#include <chrono>
#include <cstdint>
#include <type_traits>

#include <userver/proto-structs/impl/chrono_utils.hpp>
#include <userver/utils/impl/internal_tag_fwd.hpp>
#include <userver/utils/meta_light.hpp>

namespace google::protobuf {
class Duration;
}  // namespace google::protobuf

USERVER_NAMESPACE_BEGIN

namespace proto_structs {

/// @brief Type to represent `google.protobuf.Duration` in proto structs.
///
/// This type is organized in the same way as its protobuf counterpart and allows for the same range of values
/// (approximately `+-10'000` years) as described here
/// https://github.com/protocolbuffers/protobuf/blob/main/src/google/protobuf/duration.proto .
class Duration {
public:
    using ProtobufMessage = ::google::protobuf::Duration;

    /// @brief Minimum allowed number of seconds
    /// @note Appoximately `-10'000` years.
    static constexpr std::chrono::seconds kMinSeconds{-315'576'000'000LL};

    /// @brief Maximum allowed number of seconds
    /// @note Appoximately `10'000` years.
    static constexpr std::chrono::seconds kMaxSeconds{315'576'000'000LL};

    /// @brief Creates zero duration.
    constexpr Duration() = default;

    /// @brief Creates duration from @a seconds and @a nanos which are the fractions of second (from `-999'999'999` to
    ///        `999'999'999`) of the same sign as @a seconds.
    /// @throws ValueError if @a seconds and @a nanos does not have the same sign (when @a seconds is not `0`) or
    ///         outside the allowed range: `[-315'576'000'000, 315'576'000'000]` for @a seconds and
    ///         `[-999'999'999, 999'999'999]` for @a nanos.
    constexpr Duration(const std::chrono::seconds& seconds, const std::chrono::nanoseconds& nanos)
        : seconds_(seconds),
          nanos_(nanos)
    {
        if (!IsValid(seconds_, nanos_)) {
            ThrowError(seconds_, nanos_);
        }
    }

    /// @brief Creates duration.
    /// @throws ValueError if @a duration is outside the allowed range.
    template <typename TRep, typename TPeriod>
    constexpr Duration(const std::chrono::duration<TRep, TPeriod>& duration) {
        if (!impl::CanCastToSeconds(duration)) {
            ThrowError("Cast from 'std::chrono::duration' will overflow/underflow");
        }

        seconds_ = std::chrono::duration_cast<std::chrono::seconds>(duration);
        nanos_ = std::chrono::duration_cast<std::chrono::nanoseconds>(duration - seconds_);

        if (!IsValid(seconds_, nanos_)) {
            ThrowError(seconds_, nanos_);
        }
    }

    Duration(utils::impl::InternalTag, std::int64_t seconds, std::int32_t nanos);

    /// @brief Returns seconds.
    [[nodiscard]] const std::chrono::seconds& Seconds() const noexcept { return seconds_; }

    /// @brief Returns fractions of second (from `-999'999'999` to `999'999'999`).
    [[nodiscard]] const std::chrono::nanoseconds& Nanos() const noexcept { return nanos_; }

    /// @brief Converts duration to `std::chrono::duration`.
    /// @tparam TDuration `std::chrono::duration` type
    /// @warning If stored value does not fit in the `std::chrono::duration` type it is capped at
    ///          `std::chrono::duration` min/max value.
    template <typename TDuration>
    requires ::meta::kIsInstantiationOf<std::chrono::duration, TDuration>
    [[nodiscard]] constexpr TDuration ToChronoDuration() const noexcept {
        if (FitsInChronoDuration<TDuration>()) {
            return std::chrono::duration_cast<TDuration>(seconds_) + std::chrono::duration_cast<TDuration>(nanos_);
        } else {
            return seconds_.count() > 0 ? TDuration::max() : TDuration::min();
        }
    }

    /// @brief Converts duration to `std::chrono::nanoseconds`.
    /// @warning If stored value does not fit in the `std::chrono::nanoseconds` type it is capped at
    ///          `std::chrono::nanoseconds` min/max value.
    [[nodiscard]] std::chrono::nanoseconds ToNanos() const noexcept {
        return ToChronoDuration<std::chrono::nanoseconds>();
    }

    /// @brief Converts duration to `std::chrono::microseconds`.
    [[nodiscard]] std::chrono::microseconds ToMicros() const noexcept {
        // microsecond range is a superset of the valid duration range, no need to check cast
        return std::chrono::duration_cast<std::chrono::microseconds>(seconds_) +
               std::chrono::duration_cast<std::chrono::microseconds>(nanos_);
    }

    /// @brief Converts duration to `std::chrono::milliseconds`.
    [[nodiscard]] std::chrono::milliseconds ToMillis() const noexcept {
        // millisecond range is a superset of the valid duration range, no need to check cast
        return std::chrono::duration_cast<std::chrono::milliseconds>(seconds_) +
               std::chrono::duration_cast<std::chrono::milliseconds>(nanos_);
    }

    /// @brief Converts duration to `std::chrono::seconds`.
    /// Simply returns stored seconds, added for completeness.
    [[nodiscard]] std::chrono::seconds ToSeconds() const noexcept { return seconds_; }

    /// @brief Returns `true` if duration value fits in to the `std::chrono::duration` type.
    /// @tparam TDuration `std::chrono::duration` type
    template <typename TDuration>
    requires ::meta::kIsInstantiationOf<std::chrono::duration, TDuration>
    [[nodiscard]] constexpr bool FitsInChronoDuration() const noexcept {
        using Ratio = std::ratio_divide<std::nano, typename TDuration::period>;
        static_assert(
            std::is_signed_v<typename TDuration::rep> && (Ratio::num == 1 || Ratio::den == 1),
            "unsupported duration type (use well-known std types)"
        );

        if (!impl::CanCastFromSeconds<TDuration>(seconds_)) {
            return false;
        }

        TDuration remainder;

        if (seconds_.count() < 0) {
            remainder = std::chrono::duration_cast<TDuration>(seconds_) - TDuration::min();
        } else {
            remainder = TDuration::max() - std::chrono::duration_cast<TDuration>(seconds_);
        }

        if constexpr (Ratio::num == 1) {
            // 'TDuration' unit is bigger than nanosecond
            return remainder.count() - (nanos_.count() / Ratio::den) >= 0;
        } else {
            // 'TDuration' unit is smaller than nanosecond
            return (remainder.count() / Ratio::num) - nanos_.count() >= 0;
        }
    }

    /// @brief Explicit conversion to `std::chrono::duration`.
    /// @warning If stored value does not fit in the `std::chrono::duration` type it is capped at
    ///          `std::chrono::duration` min/max value.
    template <typename TDuration>
    requires ::meta::kIsInstantiationOf<std::chrono::duration, TDuration>
    [[nodiscard]] constexpr operator TDuration() const noexcept {
        return ToChronoDuration<TDuration>();
    }

    /// @brief Default three-way comparison operator.
    auto operator<=>(const Duration&) const = default;

    /// @brief Returns minimum allowed duration.
    [[nodiscard]] static constexpr Duration Min() noexcept {
        Duration result;
        result.seconds_ = kMinSeconds;
        result.nanos_ = std::chrono::nanoseconds{-999'999'999};
        return result;
    }

    /// @brief Returns maximum allowed duration.
    [[nodiscard]] static constexpr Duration Max() noexcept {
        Duration result;
        result.seconds_ = kMaxSeconds;
        result.nanos_ = std::chrono::nanoseconds{999'999'999};
        return result;
    }

    /// Returns `true` if @a seconds and @a nanos represent a valid `google.protobuf.Duration` value.
    [[nodiscard]] static constexpr bool IsValid(
        const std::chrono::seconds& seconds,
        const std::chrono::nanoseconds& nanos
    ) noexcept {
        if (seconds < kMinSeconds || seconds > kMaxSeconds) {
            return false;
        }

        if (nanos < std::chrono::nanoseconds{-999'999'999} || nanos > std::chrono::nanoseconds{999'999'999}) {
            return false;
        }

        if ((seconds.count() > 0 && nanos.count() < 0) || (seconds.count() < 0 && nanos.count() > 0)) {
            return false;
        }

        return true;
    }

private:
    [[noreturn]] static void ThrowError(const std::chrono::seconds& seconds, const std::chrono::nanoseconds& nanos);
    [[noreturn]] static void ThrowError(const char* reason);

    std::chrono::seconds seconds_{0};
    std::chrono::nanoseconds nanos_{0};
};

}  // namespace proto_structs

USERVER_NAMESPACE_END
