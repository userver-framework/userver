#pragma once

/// @file userver/utils/hedging_settings.hpp
/// @brief @copybrief utils::hedging::HedgingSettings

#include <chrono>
#include <cstddef>

USERVER_NAMESPACE_BEGIN

namespace utils::hedging {

/// Class define hedging settings
struct HedgingSettings {
    /// Maximum requests to do
    std::size_t max_attempts{3};
    /// Delay between attempts
    std::chrono::milliseconds hedging_delay{7};
    /// Max time to wait for all requests
    std::chrono::milliseconds timeout_all{100};
};

}  // namespace utils::hedging

USERVER_NAMESPACE_END
