#pragma once

#include <taxi_config/config.hpp>
#include <taxi_config/value.hpp>

namespace server_settings {

struct ConfigNamesDefault {
  static constexpr const char* kLogRequest = "USERVER_LOG_REQUEST";
  static constexpr const char* kLogRequestHeaders =
      "USERVER_LOG_REQUEST_HEADERS";
  static constexpr const char* kCheckAuthInHandlers =
      "USERVER_CHECK_AUTH_IN_HANDLERS";
};

template <typename ConfigNames = ConfigNamesDefault>
class HttpServerSettingsConfig final {
 public:
  explicit HttpServerSettingsConfig(const taxi_config::DocsMap& docs_map)
      : need_log_request(ConfigNames::kLogRequest, docs_map),
        need_log_request_headers(ConfigNames::kLogRequestHeaders, docs_map),
        need_check_auth_in_handlers(ConfigNames::kCheckAuthInHandlers,
                                    docs_map) {}

  taxi_config::Value<bool> need_log_request;
  taxi_config::Value<bool> need_log_request_headers;
  taxi_config::Value<bool> need_check_auth_in_handlers;
};

}  // namespace server_settings
