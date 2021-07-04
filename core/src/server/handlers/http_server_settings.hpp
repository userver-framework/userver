#pragma once

#include <userver/taxi_config/snapshot.hpp>
#include <userver/taxi_config/value.hpp>

namespace server::handlers {

class HttpServerSettings final {
 public:
  static HttpServerSettings Parse(const taxi_config::DocsMap& docs_map);

  bool need_log_request;
  bool need_log_request_headers;
  bool need_check_auth_in_handlers;
  bool need_cancel_handle_request_by_deadline;
};

inline constexpr taxi_config::Key<HttpServerSettings::Parse>
    kHttpServerSettings;

}  // namespace server::handlers
