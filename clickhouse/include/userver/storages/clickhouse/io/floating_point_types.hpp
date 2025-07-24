#pragma once

/// @file userver/storages/clickhouse/io/floating_point_types.hpp
/// @brief @copybrief storages::clickhouse::io::FloatingWithPrecision

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io {

/// @brief Class that can be escaped for clickhouse queries instead of [double] and [float].
/// [double] and [float] are escaped as FloatingWithPrecision<double, 12> and
/// FloatingWithPrecision<float, 6> respectively. These constants were derived from the accuracity the numbers from
/// ranges [0.0-100000.0] (doubles) and [0.0-100.0] (floats) can be represented by these type. Clickhouse also supports
/// inf, -inf and nan, see clickhouse docs.
template <
    typename FloatingPointT,
    std::uint32_t Precision = std::is_same_v<FloatingPointT, float> ? 6 : 12,
    typename = std::enable_if_t<std::is_floating_point_v<FloatingPointT>>>
class FloatingWithPrecision {
public:
    template <typename U>
    FloatingWithPrecision(U value) : value_(value) {}

    template <typename U, std::uint32_t AnotherPrecision>
    FloatingWithPrecision(const FloatingWithPrecision<U, AnotherPrecision>& other) : value_(other.value_) {}

    template <typename U, std::uint32_t AnotherPrecision>
    FloatingWithPrecision(FloatingWithPrecision<U, AnotherPrecision>&& other) : value_(other.value_) {}

    template <typename U, std::uint32_t AnotherPrecision>
    FloatingWithPrecision& operator=(const FloatingWithPrecision<U, AnotherPrecision>& other) {
        value_ = other.value_;
        return *this;
    }

    template <typename U, std::uint32_t AnotherPrecision>
    FloatingWithPrecision& operator=(FloatingWithPrecision<U, AnotherPrecision>&& other) {
        if (this == &other) {
            return *this;
        }
        value_ = other.value_;
        return *this;
    }

    std::string ToString() { return fmt::format("{:.{}f}", value_, Precision); }

private:
    FloatingPointT value_;

    template <typename U, std::uint32_t AnotherPrecision, typename>
    friend class FloatingWithPrecision;
};

}  // namespace storages::clickhouse::io

USERVER_NAMESPACE_END
