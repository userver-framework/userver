#pragma once

/// @file userver/utils/boost_uuid7s.hpp
/// @brief @copybrief utils::generators::GenerateBoostUuidV7()

#include <chrono>

#include <boost/uuid/uuid.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace generators {

/// @brief Generates UUIDv7
///
/// Uses 22-bit counter to ensure UUID's monotonicity in generated batches
/// or for the same timestamp and timestamp incrementation as
/// counter rollover handler.
///
/// See RFC for detailed info:
/// https://datatracker.ietf.org/doc/html/rfc9562#name-uuid-version-7
/// https://datatracker.ietf.org/doc/html/rfc9562#monotonicity_counters
boost::uuids::uuid GenerateBoostUuidV7();

}  // namespace generators

/// @brief Extracts timestamp from UUIDv7
///
/// Returns point in time when uuid was generated.
///
/// @note Due to implementation details time point may be inaccurate:
/// - implementation uses coarse clock, which have milliseconds precision
/// (https://www.kernel.org/doc/html/latest/core-api/timekeeping.html)
/// - implementation may move timestamp forward in order to ensure monotonicity
/// of generated uuids
///
std::chrono::system_clock::time_point ExtractTimestampFromUuidV7(boost::uuids::uuid uuid);

}  // namespace utils

USERVER_NAMESPACE_END
