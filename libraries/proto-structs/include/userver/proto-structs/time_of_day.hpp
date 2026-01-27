#pragma once

/// @file userver/proto-structs/time_of_day.hpp
/// @brief @copybrief proto_structs::TimeOfDay

#include <chrono>
#include <cstdint>

#include <userver/proto-structs/duration.hpp>
#include <userver/proto-structs/impl/chrono_utils.hpp>
#include <userver/utils/impl/internal_tag_fwd.hpp>
#include <userver/utils/meta_light.hpp>
#include <userver/utils/time_of_day.hpp>

namespace google::type {
class TimeOfDay;
}  // namespace google::type

USERVER_NAMESPACE_BEGIN

namespace proto_structs {

namespace impl {

template <typename T>
struct ChronoTimeOfDayDuration {
    using type = void;
};

template <typename TDuration>
struct ChronoTimeOfDayDuration<std::chrono::hh_mm_ss<TDuration>> {
    using type = TDuration;
};

}  // namespace impl

/// @brief Type to represent `google.type.TimeOfDay` in proto structs.
///
/// This type is organized in the same way as its protobuf counterpart and allows for the same range of values
/// described here https://github.com/googleapis/googleapis/blob/master/google/type/timeofday.proto : from `00:00:00.0`
/// to `24:00:00.0` inclusive, with leap seconds support (represented as value `60` for seconds).
class TimeOfDay {
public:
    using ProtobufMessage = ::google::type::TimeOfDay;

    /// @brief Creates `00:00:00.0` time of the day.
    constexpr TimeOfDay() = default;

    /// @brief Creates time of the day from components.
    /// @throws ValueError if some component is outside the allowed range:
    ///         * `[0, 24]` for @a hours (`24` is valid only if all other components are zeros)
    ///         * `[0, 59]` for @a minutes
    ///         * `[0, 60]` for @a seconds (`60` may be used by APIs which need leap seconds)
    ///         * `[0, 999'999'999]` for @a nanos
    constexpr TimeOfDay(
        const std::chrono::hours& hours,
        const std::chrono::minutes& minutes,
        const std::chrono::seconds& seconds,
        const std::chrono::nanoseconds& nanos = std::chrono::nanoseconds{0}
    )
        : hours_(hours),
          minutes_(minutes),
          seconds_(seconds),
          nanos_(nanos)
    {
        if (!IsValid(hours_, minutes_, seconds_, nanos_)) {
            ThrowError(hours_, minutes_, seconds_, nanos_);
        }
    }

    /// @brief Creates time of the day.
    /// @throws ValueError if @a hms is outside the allowed range.
    /// @note This constructor does not check whether @a hms contains a leap second to represent it accordingly (i.e.
    ///       with seconds part equal `60`). Use overload which explicitly accepts components of the time of the day.
    template <typename TRep, typename TPeriod>
    constexpr TimeOfDay(const std::chrono::hh_mm_ss<std::chrono::duration<TRep, TPeriod>>& hms)
        : TimeOfDay(
              hms.hours(),
              hms.minutes(),
              hms.seconds(),
              std::chrono::duration_cast<std::chrono::nanoseconds>(hms.subseconds())
          )
    {
        if (hms.is_negative()) {
            ThrowError("'std::chrono::hh_mm_ss' is negative");
        }
    }

    /// @brief Creates time of the day
    template <typename TRep, typename TPeriod>
    constexpr TimeOfDay(const utils::datetime::TimeOfDay<std::chrono::duration<TRep, TPeriod>>& time_of_day) noexcept
        : hours_(time_of_day.Hours()),
          minutes_(time_of_day.Minutes()),
          seconds_(time_of_day.Seconds()),
          nanos_(std::chrono::duration_cast<std::chrono::nanoseconds>(time_of_day.Subseconds())) {
        // userver::utils::datetime::TimeOfday supported range of values is a subset of proto_structs::TimeOfDay
        // range, no need to check time of the day components
    }

    /// @brief Creates time of the day from the @a duration.
    /// @throws ValueError if @a duration is out of range.
    template <typename TRep, typename TPeriod>
    constexpr explicit TimeOfDay(std::chrono::duration<TRep, TPeriod> duration) {
        constexpr std::chrono::seconds kSecondsInDay{60 * 60 * 24};

        if (!impl::CanCastToSeconds(duration)) {
            ThrowError("'std::chrono::duration' is out of range");
        }

        const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);

        if (seconds < std::chrono::seconds{0} || seconds > kSecondsInDay) {
            ThrowError("'std::chrono::duration' is out of range");
        }

        // now we can be sure conversions will not overflow
        *this = TimeOfDay(std::chrono::hh_mm_ss{duration});
    }

    /// @brief Creates time of the day evaluating it since the midnight before @a time_point.
    constexpr explicit TimeOfDay(const std::chrono::system_clock::time_point& time_point) noexcept
        : TimeOfDay(time_point - std::chrono::time_point_cast<std::chrono::days>(time_point)) {}

    TimeOfDay(
        utils::impl::InternalTag,
        std::int32_t hours,
        std::int32_t minutes,
        std::int32_t seconds,
        std::int32_t nanos
    );

    /// @brief Returns hours (from `0` to `24`).
    ///
    /// Special value `24` may be use in scenarios like business closing time.
    [[nodiscard]] constexpr const std::chrono::hours& Hours() const noexcept { return hours_; }

    /// @brief Returns minutes (from `0` to `59`).
    [[nodiscard]] constexpr const std::chrono::minutes& Minutes() const noexcept { return minutes_; }

    /// @brief Returns seconds (from `0` to `59` in most cases, however `60` is also valid to support APIs with
    ///        leap-seconds support).
    [[nodiscard]] constexpr const std::chrono::seconds& Seconds() const noexcept { return seconds_; }

    /// @brief Returns nanoseconds (from `0` to `999'999'999`).
    [[nodiscard]] constexpr const std::chrono::nanoseconds& Nanos() const noexcept { return nanos_; }

    /// @brief Converts time of the day to `std::chrono::hh_mm_ss`.
    template <typename TDuration = std::chrono::nanoseconds>
    requires ::meta::kIsInstantiationOf<std::chrono::duration, TDuration>
    [[nodiscard]] constexpr std::chrono::hh_mm_ss<TDuration> ToChronoTimeOfDay() const noexcept {
        return std::chrono::hh_mm_ss<TDuration>{ToChronoDuration<TDuration>()};
    }

    /// @brief Converts time of the day to `std::chrono::duration`.
    template <typename TDuration = std::chrono::nanoseconds>
    requires ::meta::kIsInstantiationOf<std::chrono::duration, TDuration>
    [[nodiscard]] constexpr TDuration ToChronoDuration() const noexcept {
        return std::chrono::duration_cast<TDuration>(hours_ + minutes_ + seconds_ + nanos_);
    }

    /// @brief Converts time of the day to `userver::utils::datetime::TimeOfDay`.
    template <typename TDuration = std::chrono::nanoseconds>
    requires ::meta::kIsInstantiationOf<std::chrono::duration, TDuration>
    [[nodiscard]] constexpr utils::datetime::TimeOfDay<TDuration> ToUserverTimeOfDay() const noexcept {
        return utils::datetime::TimeOfDay<TDuration>{ToChronoDuration<TDuration>()};
    }

    /// @brief Converts time of the day to `userver::proto_structs::Duration`.
    [[nodiscard]] constexpr Duration ToUserverDuration() const noexcept {
        return Duration{hours_ + minutes_ + seconds_ + nanos_};
    }

    /// @brief Explicit conversion to `std::chrono::hh_mm_ss`.
    template <typename TTimeOfDay>
    requires ::meta::kIsInstantiationOf<std::chrono::hh_mm_ss, TTimeOfDay>
    [[nodiscard]] constexpr explicit operator TTimeOfDay() const noexcept {
        return ToChronoTimeOfDay<typename impl::ChronoTimeOfDayDuration<TTimeOfDay>::type>();
    }

    /// @brief Explicit conversion to `std::chrono::duration`.
    template <typename TDuration = std::chrono::nanoseconds>
    requires ::meta::kIsInstantiationOf<std::chrono::duration, TDuration>
    [[nodiscard]] constexpr explicit operator TDuration() const noexcept {
        return ToChronoDuration<TDuration>();
    }

    /// @brief Explicit conversion to `userver::utils::datetime::TimeOfDay`.
    template <typename TTimeOfDay>
    requires ::meta::kIsInstantiationOf<utils::datetime::TimeOfDay, TTimeOfDay>
    [[nodiscard]] constexpr explicit operator TTimeOfDay() const noexcept {
        return ToUserverTimeOfDay<typename TTimeOfDay::DurationType>();
    }

    /// @brief Explicit conversion to `userver::proto_structs::Duration`.
    [[nodiscard]] constexpr explicit operator Duration() const noexcept {
        return Duration{hours_ + minutes_ + seconds_ + nanos_};
    }

    /// @brief Default three-way comparison operator.
    auto operator<=>(const TimeOfDay&) const = default;

    /// @brief Returns `true` if all components represent a valid `google.type.TimeOfDay` value.
    template <typename TRep, typename TPeriod>
    [[nodiscard]] static constexpr bool IsValid(
        const std::chrono::hours& hours,
        const std::chrono::minutes& minutes,
        const std::chrono::seconds& seconds,
        const std::chrono::duration<TRep, TPeriod>& subseconds
    ) noexcept {
        using SubsecondDuration = std::chrono::duration<TRep, TPeriod>;
        static_assert(TPeriod::num <= TPeriod::den, "Subsecond duration should be fraction of seconds");

        if (hours.count() != 24) {
            return hours.count() >= 0 && hours.count() <= 23 && minutes.count() >= 0 && minutes.count() <= 59 &&
                   seconds.count() >= 0 && seconds.count() <= 60 /* for leap second */
                   && subseconds.count() >= 0 &&
                   subseconds.count() <=
                       (std::chrono::duration_cast<SubsecondDuration>(std::chrono::seconds{1}).count() - 1);
        } else {
            return minutes.count() == 0 && seconds.count() == 0 && subseconds.count() == 0;
        }
    }

private:
    [[noreturn]] static void ThrowError(
        const std::chrono::hours& hours,
        const std::chrono::minutes& minutes,
        const std::chrono::seconds& seconds,
        const std::chrono::nanoseconds& nanos
    );
    [[noreturn]] static void ThrowError(const char* reason);

    std::chrono::hours hours_{0};
    std::chrono::minutes minutes_{0};
    std::chrono::seconds seconds_{0};
    std::chrono::nanoseconds nanos_{0};
};

}  // namespace proto_structs

USERVER_NAMESPACE_END
