#pragma once

/// @file userver/utils/statistics/solomon.hpp
/// @brief Statistics output in Solomon format.

#include <string>

#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// Output `statistics` in Solomon format with tags (labels). It's JSON format
/// in the form
/// @code

///{
///  "commonLabels": {
///    "application": "some-service-name"
///  },
///  "metrics": [
///    {
///      "labels": {
///        "sensor": "cpu_time_sec"
///      },
///      "value": 34155.69
///    },
///    {
///      "labels": {
///        "sensor": "postgresql.replication-lag.min",
///        "postgresql_database": "some_pg_db",
///        "postgresql_database_shard": "shard_0",
///        "postgresql_cluster_host_type": "slaves",
///        "postgresql_instance": "some_host:6432"
///      },
///      "value": 0
///    },
///    {
///      "labels": {
///        "sensor": "http.handler.in-flight",
///        "http_path": "/ping",
///        "http_handler": "handler-ping"
///      },
///      "value": 1
///    },
///    {
///      "labels": {
///        "sensor": "cache.full.time.last-update-duration-ms",
///        "cache_name": "some-cache-name"
///      },
///      "value": 23
///    }
///  ]
///}

/// @endcode

std::string ToSolomonFormat(
    const utils::statistics::Storage& statistics,
    const std::unordered_map<std::string, std::string>& common_labels,
    const utils::statistics::Request& statistics_request = {});

}  // namespace utils::statistics

USERVER_NAMESPACE_END
