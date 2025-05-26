#include <userver/utils/datetime.hpp>

#include <ctime>
#include <optional>
#include <string>

#include <sys/param.h>

#include <cctz/time_zone.h>

#include <userver/engine/task/current_task.hpp>
#include <userver/rcu/rcu_map.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/from_string.hpp>
#include <userver/utils/mock_now.hpp>
#include <userver/utils/numeric_cast.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {

namespace {

std::optional<cctz::time_zone> DoGetOptionalTimezone(const std::string& tzname) {
#if defined(BSD) && !defined(__APPLE__)
    if (tzname == "GMT") return GetOptionalTimezone("UTC");
#endif
    cctz::time_zone tz;
    if (!load_time_zone(tzname, &tz)) {
        return std::nullopt;
    }
    return tz;
}

std::optional<cctz::time_zone> GetOptionalTimezone(const std::string& tzname) {
    if (engine::current_task::IsTaskProcessorThread()) {
        static rcu::RcuMap<std::string, std::optional<cctz::time_zone>> map;
        auto it = map.Get(tzname);
        if (it) return *it;

        // DoGetOptionalTimezone() may access filesystem, run it in blocking task processor
        auto [value, _] =
            map.Emplace(tzname, engine::AsyncNoSpan(engine::current_task::GetBlockingTaskProcessor(), [&tzname] {
                                    return DoGetOptionalTimezone(tzname);
                                }).Get());
        return *value;
    } else {
        return DoGetOptionalTimezone(tzname);
    }
}

cctz::time_zone GetTimezone(const std::string& tzname) {
    auto tz = GetOptionalTimezone(tzname);
    if (!tz.has_value()) {
        throw TimezoneLookupError(tzname);
    }
    return *tz;
}
}  // namespace

std::string
Timestring(std::chrono::system_clock::time_point tp, const std::string& timezone, const std::string& format) {
    return cctz::format(format, tp, GetTimezone(timezone));
}

std::optional<std::chrono::system_clock::time_point>
OptionalStringtime(const std::string& timestring, const std::string& timezone, const std::string& format) {
    auto tz = GetOptionalTimezone(timezone);
    if (!tz.has_value()) {
        return std::nullopt;
    }
    return OptionalStringtime(timestring, *tz, format);
}

std::string Timestring(time_t timestamp, const std::string& timezone, const std::string& format) {
    auto tp = std::chrono::system_clock::from_time_t(timestamp);
    return Timestring(tp, timezone, format);
}

std::chrono::system_clock::time_point
Stringtime(const std::string& timestring, const std::string& timezone, const std::string& format) {
    const auto optional_tp = OptionalStringtime(timestring, GetTimezone(timezone), format);
    if (!optional_tp) {
        throw DateParseError(timestring);
    }
    return *optional_tp;
}

std::chrono::system_clock::time_point GuessStringtime(const std::string& timestamp, const std::string& timezone) {
    return DoGuessStringtime(timestamp, GetTimezone(timezone));
}

cctz::civil_second Localize(const std::chrono::system_clock::time_point& tp, const std::string& timezone) {
    return cctz::convert(tp, GetTimezone(timezone));
}

time_t Unlocalize(const cctz::civil_second& local_tp, const std::string& timezone) {
    return Timestamp(cctz::convert(local_tp, GetTimezone(timezone)));
}

}  // namespace utils::datetime

USERVER_NAMESPACE_END
