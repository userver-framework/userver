#pragma once

/// @file userver/utils/time_of_day.hpp
/// @brief @copybrief utils::datetime::TimeOfDay

#include <algorithm>
#include <array>
#include <chrono>
#include <string_view>
#include <type_traits>
#include <vector>

#include <fmt/format.h>

#include <userver/utils/fmt_compat.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {
class LogHelper;
}

namespace utils::datetime {

/// @ingroup userver_containers
///
/// @brief A simple implementation of a "time since midnight" datatype.
///
/// This type is time-zone ignorant.
///
/// Valid time range is from [00:00:00.0, 24:00:00.0)
///
/// Available construction:
///
/// from duration (since midnight, the value will be normalized, e.g. 25:00 will
/// become 01:00, 24:00 will become 00:00)
///
/// from string representation HH:MM[:SS[.s]], accepted range is from 00:00 to
/// 24:00
///
/// construction from int in form 1300 as a static member function
///
/// Accessors:
///
/// int Hours(); // hours since midnight
/// int Minutes(); // minutes since midnight + Hours()
/// int Seconds(); // seconds since midnight + Hours() + Minutes()
/// int Subseconds(); // seconds fraction since
///                     midnight + Hours() + Minutes() + Seconds()
///
///
/// Output:
///
/// LOG(xxx) << val
///
/// Formatting:
///
/// fmt::format("{}", val)
///
/// Default format for hours and minutes is HH:MM, for seconds HH:MM:SS, for
/// subseconds HH:MM:SS.s with fractional part, but truncating trailing zeros.
///
/// Custom formatting:
///
/// fmt:format("{:%H%M%S}", val); // will output HHMMSS without separators
///
/// Format keys:
/// %H 24-hour two-digit zero-padded hours
/// %M two-digit zero-padded minutes
/// %S two-digit zero-padded seconds
/// %% literal %

template <typename Duration>
class TimeOfDay;

template <typename Rep, typename Period>
class TimeOfDay<std::chrono::duration<Rep, Period>> {
 public:
  using DurationType = std::chrono::duration<Rep, Period>;

  constexpr TimeOfDay() noexcept = default;
  constexpr explicit TimeOfDay(DurationType) noexcept;
  template <typename ORep, typename OPeriod>
  constexpr explicit TimeOfDay(std::chrono::duration<ORep, OPeriod>) noexcept;
  constexpr explicit TimeOfDay(std::string_view);

  //@{
  /** @name Comparison operators */
  constexpr bool operator==(const TimeOfDay&) const;
  constexpr bool operator!=(const TimeOfDay&) const;
  constexpr bool operator<(const TimeOfDay&) const;
  constexpr bool operator<=(const TimeOfDay&) const;
  constexpr bool operator>(const TimeOfDay&) const;
  constexpr bool operator>=(const TimeOfDay&) const;
  //@}

  //@{
  /** @name Accessors */
  /// @return Hours since midnight
  constexpr std::chrono::hours Hours() const noexcept;
  /// @return Minutes since midnight + Hours
  constexpr std::chrono::minutes Minutes() const noexcept;
  /// @return Seconds since midnight + Hours + Minutes
  constexpr std::chrono::seconds Seconds() const noexcept;
  /// @return Fractional part of seconds since midnight + Hours + Minutes +
  /// Seconds up to resolution
  constexpr DurationType Subseconds() const noexcept;

  /// @return Underlying duration representation
  constexpr DurationType SinceMidnight() const noexcept;
  //@}

  /// Create time of day from integer representation, e.g.1330 => 13:30
  constexpr static TimeOfDay FromHHMMInt(int);

 private:
  DurationType since_midnight_{};
};

//@{
/** @name Duration arithmetic */

template <typename LDuration, typename RDuration>
auto operator-(TimeOfDay<LDuration> lhs, TimeOfDay<RDuration> rhs) {
  return lhs.SinceMidnight() - rhs.SinceMidnight();
}

template <typename Duration, typename Rep, typename Period>
TimeOfDay<Duration> operator+(TimeOfDay<Duration> lhs,
                              std::chrono::duration<Rep, Period> rhs) {
  return TimeOfDay<Duration>{lhs.SinceMidnight() + rhs};
}

template <typename Duration, typename Rep, typename Period>
TimeOfDay<Duration> operator-(TimeOfDay<Duration> lhs,
                              std::chrono::duration<Rep, Period> rhs) {
  return TimeOfDay<Duration>{lhs.SinceMidnight() - rhs};
}
//@}

template <typename Duration>
logging::LogHelper& operator<<(logging::LogHelper& lh,
                               TimeOfDay<Duration> value) {
  lh << fmt::to_string(value);
  return lh;
}

namespace detail {
template <typename Rep, typename Period>
inline constexpr std::chrono::duration<Rep, Period> kTwentyFourHours =
    std::chrono::duration_cast<std::chrono::duration<Rep, Period>>(
        std::chrono::hours{24});

template <typename Rep, typename Period, typename ORep = Rep,
          typename OPeriod = Period>
constexpr std::chrono::duration<Rep, Period> NormalizeTimeOfDay(
    std::chrono::duration<ORep, OPeriod> d) {
  auto res = std::chrono::duration_cast<std::chrono::duration<Rep, Period>>(d) %
             kTwentyFourHours<Rep, Period>;
  return res.count() < 0 ? res + kTwentyFourHours<Rep, Period> : res;
}

template <typename Ratio>
inline constexpr std::size_t kDecimalPositions = 0;
template <>
inline constexpr std::size_t kDecimalPositions<std::milli> = 3;
template <>
inline constexpr std::size_t kDecimalPositions<std::micro> = 6;
template <>
inline constexpr const std::size_t kDecimalPositions<std::nano> = 9;

constexpr std::intmax_t MissingDigits(std::size_t n) {
  // As we support resolutions up to nano, all wee need is up to 10^9
  // clang-format off
  constexpr std::intmax_t powers[]{
      1,
      10,
      100,
      1'000,
      10'000,
      100'000,
      1'000'000,
      10'000'000,
      100'000'000,
      1'000'000'000
  };
  // clang-format on

  return powers[n];
}

template <typename Ratio>
struct HasMinutes : std::false_type {};

template <intmax_t Num, intmax_t Den>
struct HasMinutes<std::ratio<Num, Den>>
    : std::integral_constant<bool, (Num <= 60LL)> {};

template <typename Rep, typename Period>
struct HasMinutes<std::chrono::duration<Rep, Period>> : HasMinutes<Period> {};

template <typename Duration>
struct HasMinutes<TimeOfDay<Duration>> : HasMinutes<Duration> {};

template <typename T>
constexpr inline bool kHasMinutes = HasMinutes<T>{};

template <typename Ratio>
struct HasSeconds : std::false_type {};

template <intmax_t Num, intmax_t Den>
struct HasSeconds<std::ratio<Num, Den>>
    : std::integral_constant<bool, (Num == 1)> {};

template <typename Rep, typename Period>
struct HasSeconds<std::chrono::duration<Rep, Period>> : HasSeconds<Period> {};

template <typename Duration>
struct HasSeconds<TimeOfDay<Duration>> : HasSeconds<Duration> {};

template <typename T>
constexpr inline bool kHasSeconds = HasSeconds<T>{};

template <typename Ratio>
struct HasSubseconds : std::false_type {};

template <intmax_t Num, intmax_t Den>
struct HasSubseconds<std::ratio<Num, Den>>
    : std::integral_constant<bool, (Den > 1)> {};

template <typename Rep, typename Period>
struct HasSubseconds<std::chrono::duration<Rep, Period>>
    : HasSubseconds<Period> {};

template <typename Duration>
struct HasSubseconds<TimeOfDay<Duration>> : HasSubseconds<Duration> {};

template <typename T>
constexpr inline bool kHasSubseconds = HasSubseconds<T>{};

template <typename Rep, typename Period>
class TimeOfDayParser {
 public:
  using DurationType = std::chrono::duration<Rep, Period>;

  constexpr DurationType operator()(std::string_view str) {
    for (auto c : str) {
      switch (c) {
        case ':':
          if (position_ >= kSeconds)
            throw std::runtime_error{
                fmt::format("Extra colon in TimeOfDay string `{}`", str)};
          AssignCurrentPosition(str);
          position_ = static_cast<TimePart>(position_ + 1);
          break;
        case '.':
          if (position_ != kSeconds)
            throw std::runtime_error{fmt::format(
                "Unexpected decimal point in TimeOfDay string `{}`", str)};
          AssignCurrentPosition(str);
          position_ = static_cast<TimePart>(position_ + 1);
          break;
        default:
          if (!std::isdigit(c))
            throw std::runtime_error{fmt::format(
                "Unexpected character {} in TimeOfDay string `{}`", c, str)};
          if (position_ == kOverflow) {
            continue;
          } else if (position_ == kSubseconds) {
            if (digit_count_ >= kDecimalPositions<Period>) {
              AssignCurrentPosition(str);
              position_ = kOverflow;
              continue;
            }
          } else if (digit_count_ >= 2) {
            throw std::runtime_error{
                fmt::format("Too much digits in TimeOfDay string `{}`", str)};
          }
          ++digit_count_;
          current_number_ = current_number_ * 10 + (c - '0');
          break;
      }
    }
    if (position_ == kHour)
      throw std::runtime_error{
          fmt::format("Expected to have at least minutes after hours in "
                      "TimeOfDay string `{}`",
                      str)};
    AssignCurrentPosition(str);

    auto sum = hours_ + minutes_ + seconds_ + subseconds_;
    if (sum > kTwentyFourHours<Rep, Period>) {
      throw std::runtime_error(fmt::format(
          "TimeOfDay value {} is out of range [00:00, 24:00)", str));
    }
    return NormalizeTimeOfDay<Rep, Period>(sum);
  }

 private:
  enum TimePart { kHour, kMinutes, kSeconds, kSubseconds, kOverflow };

  void AssignCurrentPosition(std::string_view str) {
    switch (position_) {
      case kHour: {
        if (digit_count_ < 1)
          throw std::runtime_error{fmt::format(
              "Not enough digits for hours in TimeOfDay string `{}`", str)};
        if (current_number_ > 24)
          throw std::runtime_error{fmt::format(
              "Invalid value for hours in TimeOfDay string `{}`", str)};
        hours_ = std::chrono::hours{current_number_};
        break;
      }
      case kMinutes: {
        if (digit_count_ != 2)
          throw std::runtime_error{fmt::format(
              "Not enough digits for minutes in TimeOfDay string `{}`", str)};
        if (current_number_ > 59)
          throw std::runtime_error{fmt::format(
              "Invalid value for minutes in TimeOfDay string `{}`", str)};
        minutes_ = std::chrono::minutes{current_number_};
        break;
      }
      case kSeconds: {
        if (digit_count_ != 2)
          throw std::runtime_error{fmt::format(
              "Not enough digits for seconds in TimeOfDay string `{}`", str)};
        if (current_number_ > 59)
          throw std::runtime_error{fmt::format(
              "Invalid value for seconds in TimeOfDay string `{}`", str)};
        seconds_ = std::chrono::seconds{current_number_};
        break;
      }
      case kSubseconds: {
        if (digit_count_ < 1)
          throw std::runtime_error{fmt::format(
              "Not enough digits for subseconds in TimeOfDay string `{}`",
              str)};
        if constexpr (kHasSubseconds<Period>) {
          // TODO check digit count and adjust if needed
          if (digit_count_ < kDecimalPositions<Period>) {
            current_number_ *=
                MissingDigits(kDecimalPositions<Period> - digit_count_);
          }
          subseconds_ = DurationType{current_number_};
        }
        break;
      }
      case kOverflow:
        // Just ignore this
        break;
    }
    current_number_ = 0;
    digit_count_ = 0;
  }

  TimePart position_ = kHour;
  std::chrono::hours hours_{0};
  std::chrono::minutes minutes_{0};
  std::chrono::seconds seconds_{0};
  DurationType subseconds_{0};

  std::size_t digit_count_{0};
  std::size_t current_number_{0};
};

/// Format string used for format key `%H`, two-digit 24-hour left-padded by 0
inline constexpr std::string_view kLongHourFormat = "{0:0>#2d}";
/// Format string used for minutes, key `%M`, no variations
inline constexpr std::string_view kMinutesFormat = "{1:0>#2d}";
/// Format string used for seconds, key `%S`, no variations
inline constexpr std::string_view kSecondsFormat = "{2:0>#2d}";
/// Format string for subseconds, keys not yet assigned
inline constexpr std::string_view kSubsecondsFormat = "{3}";

template <typename Ratio>
constexpr inline std::string_view kSubsecondsPreformat = ".0";
template <>
inline constexpr std::string_view kSubsecondsPreformat<std::milli> =
    ".{:0>#3d}";
template <>
inline constexpr std::string_view kSubsecondsPreformat<std::micro> =
    ".{:0>#6d}";
template <>
inline constexpr std::string_view kSubsecondsPreformat<std::nano> = ".{:0>#9d}";

// Default format for formatting is HH:MM:SS
template <typename Ratio>
inline constexpr std::array<std::string_view, 5> kDefaultFormat{
    {kLongHourFormat, ":", kMinutesFormat, ":", kSecondsFormat}};

// Default format for formatting with minutes resolution is HH:MM
template <>
inline constexpr std::array<std::string_view, 3>
    kDefaultFormat<std::ratio<60, 1>>{{kLongHourFormat, ":", kMinutesFormat}};

// Default format for formatting with hours resolution is HH:MM
template <>
inline constexpr std::array<std::string_view, 3>
    kDefaultFormat<std::ratio<3600, 1>>{{kLongHourFormat, ":", kMinutesFormat}};

}  // namespace detail

template <typename Rep, typename Period>
constexpr TimeOfDay<std::chrono::duration<Rep, Period>>::TimeOfDay(
    DurationType d) noexcept
    : since_midnight_{detail::NormalizeTimeOfDay<Rep, Period>(d)} {}

template <typename Rep, typename Period>
template <typename ORep, typename OPeriod>
constexpr TimeOfDay<std::chrono::duration<Rep, Period>>::TimeOfDay(
    std::chrono::duration<ORep, OPeriod> d) noexcept
    : since_midnight_{detail::NormalizeTimeOfDay<Rep, Period>(d)} {}

template <typename Rep, typename Period>
constexpr TimeOfDay<std::chrono::duration<Rep, Period>>::TimeOfDay(
    std::string_view str)
    : since_midnight_{detail::TimeOfDayParser<Rep, Period>{}(str)} {}

template <typename Rep, typename Period>
constexpr bool TimeOfDay<std::chrono::duration<Rep, Period>>::operator==(
    const TimeOfDay& rhs) const {
  return since_midnight_ == rhs.since_midnight_;
}

template <typename Rep, typename Period>
constexpr bool TimeOfDay<std::chrono::duration<Rep, Period>>::operator!=(
    const TimeOfDay& rhs) const {
  return !(*this == rhs);
}

template <typename Rep, typename Period>
constexpr bool TimeOfDay<std::chrono::duration<Rep, Period>>::operator<(
    const TimeOfDay& rhs) const {
  return since_midnight_ < rhs.since_midnight_;
}

template <typename Rep, typename Period>
constexpr bool TimeOfDay<std::chrono::duration<Rep, Period>>::operator<=(
    const TimeOfDay& rhs) const {
  return since_midnight_ <= rhs.since_midnight_;
}

template <typename Rep, typename Period>
constexpr bool TimeOfDay<std::chrono::duration<Rep, Period>>::operator>(
    const TimeOfDay& rhs) const {
  return since_midnight_ > rhs.since_midnight_;
}

template <typename Rep, typename Period>
constexpr bool TimeOfDay<std::chrono::duration<Rep, Period>>::operator>=(
    const TimeOfDay& rhs) const {
  return since_midnight_ >= rhs.since_midnight_;
}

template <typename Rep, typename Period>
constexpr std::chrono::hours
TimeOfDay<std::chrono::duration<Rep, Period>>::Hours() const noexcept {
  return std::chrono::duration_cast<std::chrono::hours>(since_midnight_);
}

template <typename Rep, typename Period>
constexpr std::chrono::minutes
TimeOfDay<std::chrono::duration<Rep, Period>>::Minutes() const noexcept {
  if constexpr (detail::kHasMinutes<Period>) {
    return std::chrono::duration_cast<std::chrono::minutes>(since_midnight_) -
           std::chrono::duration_cast<std::chrono::minutes>(Hours());
  } else {
    return std::chrono::minutes{0};
  }
}

template <typename Rep, typename Period>
constexpr std::chrono::seconds
TimeOfDay<std::chrono::duration<Rep, Period>>::Seconds() const noexcept {
  if constexpr (detail::kHasSeconds<Period>) {
    return std::chrono::duration_cast<std::chrono::seconds>(since_midnight_) -
           std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::duration_cast<std::chrono::minutes>(
                   since_midnight_));
  } else {
    return std::chrono::seconds{0};
  }
}

template <typename Rep, typename Period>
constexpr typename TimeOfDay<std::chrono::duration<Rep, Period>>::DurationType
TimeOfDay<std::chrono::duration<Rep, Period>>::Subseconds() const noexcept {
  if constexpr (detail::kHasSubseconds<Period>) {
    return since_midnight_ -
           std::chrono::duration_cast<std::chrono::seconds>(since_midnight_);
  } else {
    return DurationType{0};
  }
}

template <typename Rep, typename Period>
constexpr typename TimeOfDay<std::chrono::duration<Rep, Period>>::DurationType
TimeOfDay<std::chrono::duration<Rep, Period>>::SinceMidnight() const noexcept {
  return since_midnight_;
}

template <typename Rep, typename Period>
constexpr TimeOfDay<std::chrono::duration<Rep, Period>>
TimeOfDay<std::chrono::duration<Rep, Period>>::FromHHMMInt(int hh_mm) {
  auto mm = hh_mm % 100;
  if (mm >= 60)
    throw std::runtime_error{fmt::format(
        "Invalid value for minutes {} in int representation {}", mm, hh_mm)};
  return TimeOfDay{std::chrono::minutes{hh_mm / 100 * 60 + mm}};
}

}  // namespace utils::datetime

USERVER_NAMESPACE_END

namespace fmt {

template <typename Duration>
class formatter<USERVER_NAMESPACE::utils::datetime::TimeOfDay<Duration>> {
  /// Format string used for format key `%H`, two-digit 24-hour left-padded by 0
  static constexpr auto kLongHourFormat =
      USERVER_NAMESPACE::utils::datetime::detail::kLongHourFormat;
  /// Format string used for minutes, key `%M`, no variations
  static constexpr auto kMinutesFormat =
      USERVER_NAMESPACE::utils::datetime::detail::kMinutesFormat;
  /// Format string used for seconds, key `%S`, no variations
  static constexpr auto kSecondsFormat =
      USERVER_NAMESPACE::utils::datetime::detail::kSecondsFormat;
  /// Format string for subseconds, keys not yet assigned
  /// for use in representation
  static constexpr auto kSubsecondsFormat =
      USERVER_NAMESPACE::utils::datetime::detail::kSubsecondsFormat;

  static constexpr auto kSubsecondsPreformat =
      USERVER_NAMESPACE::utils::datetime::detail::kSubsecondsPreformat<
          typename Duration::period>;

  static constexpr std::string_view kLiteralPercent = "%";

  static constexpr auto kDefaultFormat =
      USERVER_NAMESPACE::utils::datetime::detail::kDefaultFormat<
          typename Duration::period>;

  constexpr std::string_view GetFormatForKey(char key) {
    // TODO Check if time part already seen
    switch (key) {
      case 'H':
        return kLongHourFormat;
      case 'M':
        return kMinutesFormat;
      case 'S':
        return kSecondsFormat;
      default:
        throw format_error{fmt::format("Unsupported format key {}", key)};
    }
  }

 public:
  constexpr auto parse(format_parse_context& ctx) {
    enum { kChar, kPercent, kKey } state = kChar;
    const auto* it = ctx.begin();
    const auto* end = ctx.end();
    const auto* begin = it;

    bool custom_format = false;
    std::size_t size = 0;
    for (; it != end && *it != '}'; ++it) {
      if (!custom_format) {
        representation_size_ = 0;
        custom_format = true;
      }
      if (*it == '%') {
        if (state == kPercent) {
          PushBackFmt(kLiteralPercent);
          state = kKey;
        } else {
          if (state == kChar && size > 0) {
            PushBackFmt({begin, size});
          }
          state = kPercent;
        }
      } else {
        if (state == kPercent) {
          PushBackFmt(GetFormatForKey(*it));
          state = kKey;
        } else if (state == kKey) {
          // start new literal
          begin = it;
          size = 1;
          state = kChar;
        } else {
          ++size;
        }
      }
    }
    if (!custom_format) {
      for (const auto fmt : kDefaultFormat) {
        PushBackFmt(fmt);
      }
    }
    if (state == kChar) {
      if (size > 0) {
        PushBackFmt({begin, size});
      }
    } else if (state == kPercent) {
      throw format_error{"No format key after percent character"};
    }
    return it;
  }

  template <typename FormatContext>
  constexpr auto format(
      const USERVER_NAMESPACE::utils::datetime::TimeOfDay<Duration>& value,
      FormatContext& ctx) const {
    auto hours = value.Hours().count();
    auto mins = value.Minutes().count();
    auto secs = value.Seconds().count();

    auto ss = value.Subseconds().count();

    // Number of decimal positions (min 1) + point + null terminator
    constexpr std::size_t buffer_size =
        std::max(USERVER_NAMESPACE::utils::datetime::detail::kDecimalPositions<
                     typename Duration::period>,
                 std::size_t{1}) +
        2;
    char subseconds[buffer_size];
    subseconds[0] = 0;
    if (ss > 0 || !truncate_trailing_subseconds_) {
      format_to(subseconds, kSubsecondsPreformat, ss);
      subseconds[buffer_size - 1] = 0;
      if (truncate_trailing_subseconds_) {
        // Truncate trailing zeros
        for (auto last = buffer_size - 2; last > 0 && subseconds[last] == '0';
             --last)
          subseconds[last] = 0;
      }
    }

    auto res = ctx.out();
    for (std::size_t i = 0; i < representation_size_; ++i) {
      res = format_to(ctx.out(), fmt::runtime(representation_[i]), hours, mins,
                      secs, subseconds);
    }
    return res;
  }

 private:
  constexpr void PushBackFmt(std::string_view fmt) {
    if (representation_size_ >= kRepresentationCapacity) {
      throw format_error("Format string complexity exceeds TimeOfDay limits");
    }
    representation_[representation_size_++] = fmt;
  }

  // Enough for hours, minutes, seconds, text and some % literals.
  static constexpr std::size_t kRepresentationCapacity = 10;

  std::string_view representation_[kRepresentationCapacity]{};
  std::size_t representation_size_{0};
  bool truncate_trailing_subseconds_{true};
};

}  // namespace fmt
