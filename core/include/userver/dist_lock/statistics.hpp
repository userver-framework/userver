#pragma once

/// @file userver/dist_lock/statistics.hpp
/// @brief @copybrief dist_lock::Statistics

#include <cstddef>

#include <userver/utils/statistics/rate_counter.hpp>

USERVER_NAMESPACE_BEGIN

namespace dist_lock {

/// @brief Distributed lock worker statistics counters
struct Statistics {
    utils::statistics::RateCounter lock_successes{};
    utils::statistics::RateCounter lock_failures{};
    utils::statistics::RateCounter watchdog_triggers{};
    utils::statistics::RateCounter brain_splits{};
    utils::statistics::RateCounter task_failures{};
};

}  // namespace dist_lock

USERVER_NAMESPACE_END
