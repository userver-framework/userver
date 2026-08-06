#pragma once

/// @file userver/storages/redis/health_check_param.hpp
/// @brief @copybrief storages::redis::HealthCheckParams

#include <userver/storages/redis/wait_connected_mode.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

/// @brief Parameters for checking Redis cluster health
struct HealthCheckParams {
    /// Wait mode for checking Redis cluster health
    storages::redis::WaitConnectedMode mode = storages::redis::WaitConnectedMode::kNoWait;
    /// Maximum number of failed shards allowed.
    /// 0 means all shards must be ready.
    uint32_t max_failed_shards{0};
    /// Maximum percent of failed shards allowed.
    /// 0 means all shards must be ready.
    uint32_t max_failed_shards_percent{0};
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
