#pragma once

/// @file clients/dns/config.hpp
/// @brief @copybrief clients::dns::ResolverConfig

#include <chrono>
#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace clients::dns {

/// Caching DNS resolver static configuration.
struct ResolverConfig {
  /// hosts file path
  std::string file_path{"/etc/hosts"};

  /// hosts file update interval
  std::chrono::milliseconds file_update_interval{std::chrono::minutes{5}};

  /// Network query timeout
  std::chrono::milliseconds network_timeout{std::chrono::seconds{1}};

  /// Network query attempts
  int network_attempts{1};

  /// Custom name servers list (system-wide resolvers used if empty)
  std::vector<std::string> network_custom_servers;

  /// Network cache ways
  size_t cache_ways{16};

  /// Network cache size per way
  size_t cache_size_per_way{256};

  /// Network cache upper reply TTL limit
  std::chrono::milliseconds cache_max_reply_ttl{std::chrono::minutes{5}};

  /// Network cache failure TTL
  std::chrono::milliseconds cache_failure_ttl{std::chrono::seconds{5}};
};

}  // namespace clients::dns

USERVER_NAMESPACE_END
