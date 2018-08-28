#pragma once

#include <taxi_config/config.hpp>
#include <taxi_config/value.hpp>

namespace server {

class HttpServerSettingsConfig {
 public:
  explicit HttpServerSettingsConfig(const taxi_config::DocsMap& docs_map)
      : need_log_request("DAUTH_LOG_REQUEST", docs_map),
        need_log_request_headers("DAUTH_LOG_REQUEST_HEADERS", docs_map) {}

  taxi_config::Value<bool> need_log_request;
  taxi_config::Value<bool> need_log_request_headers;
};

}  // namespace server
