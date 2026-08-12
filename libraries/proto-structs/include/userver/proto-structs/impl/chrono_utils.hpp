#pragma once

#include <chrono>
#include <cstdint>
#include <type_traits>

#include <userver/utils/meta_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::impl {

template <typename TSource>
constexpr bool CanCastToSeconds(const TSource& duration) {
    using SourceRep = typename TSource::rep;
    using SourcePeriod = typename TSource::period;
    using Mul = std::ratio_divide<SourcePeriod, std::chrono::seconds::period>;

    static_assert(std::is_integral_v<SourceRep> && std::is_signed_v<SourceRep> && (Mul::num == 1 || Mul::den == 1));

    if constexpr (Mul::num == 1) {
        constexpr std::intmax_t kMin = std::numeric_limits<std::chrono::seconds::rep>::min();
        constexpr std::intmax_t kMax = std::numeric_limits<std::chrono::seconds::rep>::max();
        const auto value = static_cast<std::intmax_t>(duration.count() / Mul::den);
        return value >= kMin && value <= kMax;
    } else {
        constexpr std::intmax_t kMin = std::numeric_limits<std::chrono::seconds::rep>::min() / Mul::num;
        constexpr std::intmax_t kMax = std::numeric_limits<std::chrono::seconds::rep>::max() / Mul::num;
        const auto value = static_cast<std::intmax_t>(duration.count());
        return value >= kMin && value <= kMax;
    }
}

template <typename TDest>
constexpr bool CanCastFromSeconds(const std::chrono::seconds& duration) {
    using DestRep = typename TDest::rep;
    using DestPeriod = typename TDest::period;
    using Mul = std::ratio_divide<std::chrono::seconds::period, DestPeriod>;

    static_assert(std::is_integral_v<DestRep> && std::is_signed_v<DestRep> && (Mul::num == 1 || Mul::den == 1));

    if constexpr (Mul::num == 1) {
        constexpr std::intmax_t kMin = std::numeric_limits<DestRep>::min();
        constexpr std::intmax_t kMax = std::numeric_limits<DestRep>::max();
        const auto value = static_cast<std::intmax_t>(duration.count() / Mul::den);
        return value >= kMin && value <= kMax;
    } else {
        constexpr std::intmax_t kMin = std::numeric_limits<DestRep>::min() / Mul::num;
        constexpr std::intmax_t kMax = std::numeric_limits<DestRep>::max() / Mul::num;
        const auto value = static_cast<std::intmax_t>(duration.count());
        return value >= kMin && value <= kMax;
    }
}

}  // namespace proto_structs::impl

USERVER_NAMESPACE_END
