#pragma once

/// @file
/// @brief @copybrief storages::redis::ScanTag

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

/// @brief Enum that distinguished different SCAN requests for the storages::redis::ScanRequest
enum class ScanTag {
    kScan,   ///< SCAN Redis command: iterates the set of keys in the currently selected Redis database
    kSscan,  ///< SSCAN Redis command: iterates elements of Sets types
    kHscan,  ///< HSCAN Redis command: iterates fields of Hash types and their associated values
    kZscan,  ///< ZSCAN Redis command: iterates elements of Sorted Set types and their associated scores
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
