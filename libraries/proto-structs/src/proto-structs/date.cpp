#include <userver/proto-structs/date.hpp>

#include <fmt/format.h>

#include <userver/proto-structs/exceptions.hpp>
#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs {

Date::Date(utils::impl::InternalTag, std::int32_t year, std::int32_t month, std::int32_t day) {
    if (!IsValid(utils::impl::InternalTag{}, year, month, day)) {
        ThrowError(year, month, day, "invalid or out of range");
    }

    if (year != 0) {
        year_.emplace(year);
    }

    if (month != 0) {
        month_.emplace(static_cast<unsigned>(month));
    }

    if (day != 0) {
        day_.emplace(static_cast<unsigned>(day));
    }
}

bool Date::IsValid(utils::impl::InternalTag, std::int32_t year_num, std::int32_t month_num, std::int32_t day_num)
    noexcept {
    std::optional<std::chrono::year> year;
    std::optional<std::chrono::month> month;
    std::optional<std::chrono::day> day;

    if (year_num != 0) {
        year.emplace(year_num);
    }

    if (month_num != 0) {
        if (month_num > 0 && month_num <= 12) {
            month.emplace(static_cast<unsigned>(month_num));
        } else {
            return false;
        }
    }

    if (day_num != 0) {
        if (day_num > 0 && day_num <= 31) {
            day.emplace(static_cast<unsigned>(day_num));
        } else {
            return false;
        }
    }

    return IsValid(year, month, day);
}

void Date::ThrowError(std::int32_t year, std::int32_t month, std::int32_t day, const char* reason) {
    throw ValueError(fmt::format("Date '{}y/{}m/{}d' error: {}", year, month, day, reason));
}
}  // namespace proto_structs

USERVER_NAMESPACE_END
