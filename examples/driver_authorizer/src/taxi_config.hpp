#pragma once

#include <redis/base.hpp>
#include <taxi_config/value.hpp>

namespace driver_authorizer {

struct TaxiConfig {
  using DocsMap = taxi_config::DocsMap;

  explicit TaxiConfig(const DocsMap& docs_map);

  taxi_config::Value<size_t> driver_session_expire_seconds;
  taxi_config::Value<redis::CommandControl> driver_session_cc;
};

}  // namespace driver_authorizer
