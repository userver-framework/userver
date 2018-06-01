#pragma once

#include <taxi_config/value.hpp>

namespace driver_authorizer {

struct TaxiConfig {
  using DocsMap = taxi_config::DocsMap;
  template <typename T>
  using Value = taxi_config::Value<T>;

  explicit TaxiConfig(const DocsMap& docs_map);

  Value<size_t> driver_session_expire_seconds;
  Value<bool> need_log_request;
  Value<bool> need_log_request_headers;
};

}  // namespace driver_authorizer
