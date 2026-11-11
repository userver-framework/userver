#include <userver/utils/bytes_per_second.hpp>

#include <limits>
#include <stdexcept>
#include <string_view>

#include <fmt/format.h>

#include <userver/utils/str_icase.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace {

constexpr TrivialBiMap kPrefixes = [](auto selector) {
    return selector()
        .Case("b/s", 1LL)
        .Case("kbit/s", 125LL)
        .Case("kibit/s", 128LL)
        .Case("kb/s", 1000LL)
        .Case("kib/s", 1024LL)
        .Case("mbit/s", 125000LL)
        .Case("mibit/s", 131072LL)
        .Case("mb/s", 1000000LL)
        .Case("mib/s", 1048576LL)
        .Case("gbit/s", 125000000LL)
        .Case("gibit/s", 134217728LL)
        .Case("gb/s", 1000000000LL)
        .Case("gib/s", 1073741824LL)
        .Case("tbit/s", 125000000000LL)
        .Case("tibit/s", 137438953472LL)
        .Case("tb/s", 1000000000000LL)
        .Case("tib/s", 1099511627776LL);
};

}  // namespace

BytesPerSecond StringToBytesPerSecond(const std::string& data) {
    std::size_t parsed_size = 0;
    const auto new_to = std::stoll(data, &parsed_size, 10);

    if (new_to < 0) {
        throw std::logic_error(fmt::format("StringToBytesPerSecond: '{}' is negative", data));
    }

    const std::string_view remained{data.c_str() + parsed_size};
    if (const auto multiplier = kPrefixes.TryFindICase(remained)) {
        static constexpr auto kMax = std::numeric_limits<long long>::max();
        if (kMax / *multiplier < new_to) {
            throw std::runtime_error(fmt::format("{} can not be represented as B/s without precision loss", data));
        }

        return BytesPerSecond{new_to * *multiplier};
    }

    if (StrIcaseEqual{}(remained, "bit/s")) {
        if (new_to / 8 * 8 != new_to) {
            throw std::runtime_error(fmt::format("{}bit/s can not be represented as B/s", new_to));
        }

        return BytesPerSecond{new_to / 8};
    }

    throw std::logic_error(
        fmt::format("StringToBytesPerSecond: unknown format specifier '{}' in string '{}'", remained, data)
    );
}

}  // namespace utils

USERVER_NAMESPACE_END
