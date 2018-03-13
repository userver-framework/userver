#pragma once

#include <memory>

#include "value.hpp"

namespace taxi_config {

struct TaxiConfig {
  explicit TaxiConfig(const DocsMap& docs_map);

  Value<size_t> driver_session_expire_seconds;
};

using TaxiConfigPtr = std::shared_ptr<TaxiConfig>;

}  // namespace taxi_config
