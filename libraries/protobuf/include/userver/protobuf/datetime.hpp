#pragma once

#include <cstdint>

/// @file userver/protobuf/utils.hpp
/// @brief Date and time protobuf utilities.

USERVER_NAMESPACE_BEGIN

namespace protobuf {

constexpr inline std::int64_t kMinDurationSeconds = -315'576'000'000LL;
constexpr inline std::int64_t kMaxDurationSeconds = 315'576'000'000LL;
constexpr inline std::int32_t kMinDurationNanos = -999'999'999;
constexpr inline std::int32_t kMaxDurationNanos = 999'999'999;
constexpr inline std::int64_t kMinTimestampSeconds = -62'135'596'800LL;
constexpr inline std::int64_t kMaxTimestampSeconds = 253'402'300'799LL;
constexpr inline std::int32_t kMinTimestampNanos = 0;
constexpr inline std::int32_t kMaxTimestampNanos = 999'999'999;

/// @brief Returns @c true if combination of @a seconds and @a nanos represent a valid `google.protobuf.Duration`.
[[nodiscard]] bool IsValidDuration(std::int64_t seconds, std::int32_t nanos) noexcept;

/// @brief Returns @c true if combination of @a seconds and @a nanos represent a valid `google.protobuf.Timestamp`.
[[nodiscard]] bool IsValidTimestamp(std::int64_t seconds, std::int32_t nanos) noexcept;

}  // namespace protobuf

USERVER_NAMESPACE_END
