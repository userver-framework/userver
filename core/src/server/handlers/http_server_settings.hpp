#pragma once

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

class HttpServerSettings final {
 public:
  static HttpServerSettings Parse(const dynamic_config::DocsMap& docs_map);

  bool need_log_request;
  bool need_log_request_headers;
  bool need_check_auth_in_handlers;
  bool need_cancel_handle_request_by_deadline;
};

inline constexpr dynamic_config::Key<HttpServerSettings::Parse>
    kHttpServerSettings;

}  // namespace server::handlers

USERVER_NAMESPACE_END
