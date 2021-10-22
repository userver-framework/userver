#pragma once

#include <chrono>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/* Graphite stores metrics once per minute,
 * so we have to store at least 1min */
constexpr auto kDefaultMaxPeriod = std::chrono::seconds(60);

constexpr auto kDefaultEpochDuration = std::chrono::seconds(5);

std::string DurationToString(std::chrono::seconds duration);

}  // namespace utils::statistics

USERVER_NAMESPACE_END
