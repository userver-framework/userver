#pragma once

#include <chrono>

#include "component_config.hpp"

namespace components {

class CacheConfig {
 public:
  explicit CacheConfig(const ComponentConfig& config);

  const std::chrono::milliseconds update_interval_;
  const std::chrono::milliseconds update_jitter_;
  const std::chrono::milliseconds full_update_interval_;
};

}  // namespace components
