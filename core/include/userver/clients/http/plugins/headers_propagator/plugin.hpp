#pragma once

#include <userver/clients/http/request_tracing_editor.hpp>
#include <userver/server/http/http_request.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::plugins::headers_propagator {

class HeadersPropagator final {
 public:
  void PropagateHeaders(clients::http::RequestTracingEditor request) const;
};

}  // namespace clients::http::plugins::headers_propagator

USERVER_NAMESPACE_END
