#pragma once

#include <chrono>

#include <taxi_config/value.hpp>

namespace clients::http {

struct Config {
  using DocsMap = ::taxi_config::DocsMap;

  static constexpr size_t kNoLimit = -1UL;

  explicit Config(const DocsMap& docs_map);

  ::taxi_config::Value<size_t> connection_pool_size;

  size_t http_connect_throttle_limit;
  std::chrono::microseconds http_connect_throttle_rate;
  size_t https_connect_throttle_limit;
  std::chrono::microseconds https_connect_throttle_rate;
  size_t per_host_connect_throttle_limit;
  std::chrono::microseconds per_host_connect_throttle_rate;
};

}  // namespace clients::http
