#pragma once

#include <taxi_config/value.hpp>

namespace clients {
namespace http {

struct Config {
  using DocsMap = taxi_config::DocsMap;

  explicit Config(const DocsMap& docs_map);

  taxi_config::Value<size_t> threads;
  taxi_config::Value<size_t> connection_pool_size;
  size_t connect_throttle_max_size_;
  std::chrono::milliseconds connect_throttle_update_interval_;
};

}  // namespace http
}  // namespace clients
