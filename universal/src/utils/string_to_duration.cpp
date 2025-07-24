#include <userver/utils/string_to_duration.hpp>

#include <fmt/format.h>

#include <charconv>
#include <stdexcept>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace {

template <class Rep, class Period>
std::chrono::milliseconds checked_convert(std::chrono::duration<Rep, Period> d, std::string_view data) {
    const std::chrono::duration<long double, std::chrono::milliseconds::period> extended_duration{d};
    if (extended_duration > std::chrono::milliseconds::max()) {
        throw std::overflow_error(fmt::format("StringToDuration overflow while representing '{}' as ms", data));
    } else if (extended_duration < std::chrono::milliseconds::min()) {
        throw std::underflow_error(fmt::format("StringToDuration underflow while representing '{}' as ms", data));
    }

    return std::chrono::duration_cast<std::chrono::milliseconds>(d);
}

}  // namespace

std::chrono::milliseconds StringToDuration(std::string_view data) {
    std::int64_t new_to{};
    const auto* str_begin = data.data();
    const auto* str_end = str_begin + data.size();
    const auto [end, error_code] = std::from_chars(str_begin, str_end, new_to, 10);

    if (error_code != std::errc{}) {
        throw std::logic_error(fmt::format("StringToDuration: '{}' is not a base-10 number", data));
    }

    if (new_to < 0) {
        throw std::logic_error(fmt::format("StringToDuration: '{}' is negative", data));
    }

    const std::string_view remained{end, static_cast<std::size_t>(str_end - end)};

    if (remained.empty() || remained == "s") {
        return checked_convert(std::chrono::seconds{new_to}, data);
    } else if (remained == "ms") {
        return checked_convert(std::chrono::milliseconds{new_to}, data);
    } else if (remained == "m") {
        return checked_convert(std::chrono::minutes{new_to}, data);
    } else if (remained == "h") {
        return checked_convert(std::chrono::hours{new_to}, data);
    } else if (remained == "d") {
        using Days = std::chrono::duration<int64_t, std::ratio<60 * 60 * 24>>;
        return checked_convert(Days{new_to}, data);
    }

    throw std::logic_error(fmt::format("StringToDuration: unknown format specifier '{}' in string '{}'", remained, data)
    );
}

}  // namespace utils

USERVER_NAMESPACE_END
